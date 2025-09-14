#include "gatewayServer.h"
#include <chrono>
#include <sstream>
#include <functional>
#include "logger/logger.h"
#include "logger/log_init.h"

using namespace muduo;
using namespace muduo::net;

// ---------- util ----------
std::vector<std::string> GatewayServer::split(const std::string &s, char d)
{
	std::vector<std::string> v;
	std::string cur;
	for (char c : s)
	{
		if (c == d)
		{
			v.emplace_back(std::move(cur));
			cur.clear();
		}
		else
			cur.push_back(c);
	}
	v.emplace_back(std::move(cur));
	return v;
}

std::vector<std::string> GatewayServer::splitOnce(const std::string &s, char d, size_t max_parts)
{
	std::vector<std::string> v;
	size_t start = 0, cnt = 0;
	while (cnt + 1 < max_parts)
	{
		size_t pos = s.find(d, start);
		if (pos == std::string::npos)
			break;
		v.emplace_back(s.substr(start, pos - start));
		start = pos + 1;
		cnt++;
	}
	v.emplace_back(s.substr(start));
	return v;
}

void GatewayServer::sendLine(const TcpConnectionPtr &c, const std::string &s)
{
	c->send(s);
	c->send("\n");
}

GatewayServer::Session &GatewayServer::sessionOf(const TcpConnectionPtr &c)
{
	return sessions_[c->name()];
}

int GatewayServer::gatewayChannel() const
{
	// 必须和 im-presence 使用的算法一致
	// Presence服务使用: 10000 + hash(gw) % 50000
	return 10000 + static_cast<int>(std::hash<std::string>{}(gateway_id_) % 50000);
}

// ---------- ctor ----------
GatewayServer::GatewayServer(EventLoop *loop, const InetAddress &addr, const std::string &gateway_id)
	: server_(loop, addr, "im-gateway"), gateway_id_(gateway_id)
{
	server_.setConnectionCallback([this](const TcpConnectionPtr &c)
								  { onConnection(c); });
	server_.setMessageCallback([this](const TcpConnectionPtr &c, Buffer *b, Timestamp t)
							   { onMessage(c, b, t); });
	server_.setThreadNum(2); // 可按需调整

	ch_user_.reset(new MprpcChannel());
	ch_presence_.reset(new MprpcChannel());
	ch_message_.reset(new MprpcChannel());
	ch_group_.reset(new MprpcChannel());

	user_.reset(new mpim::UserService_Stub(ch_user_.get()));
	presence_.reset(new mpim::PresenceService_Stub(ch_presence_.get()));
	message_.reset(new mpim::MessageService_Stub(ch_message_.get()));
	group_.reset(new mpim::GroupService_Stub(ch_group_.get()));

	// 订阅网关通道，接收 presence.Deliver 投递的 C2C 消息（序列化后的 C2CMsg）
	if (redis_.connect())
	{
		LOG_INFO << "Gateway: Redis connected successfully";
		int ch = gatewayChannel();
		LOG_INFO << "Gateway: Subscribing to channel " << ch;
		redis_.init_notify_handler([this](int /*channel*/, std::string payload)
								   {
			LOG_DEBUG << "Gateway: Received message from Redis, payload size=" << payload.size();
            mpim::C2CMsg m;
            if(!m.ParseFromString(payload)) {
				LOG_WARN << "bad payload for C2CMsg";
                return;
            }
			LOG_DEBUG << "Gateway: Parsed message to=" << m.to() << " from=" << m.from() << " text=" << m.text();
            auto it = uid2conn_.find(m.to());
            if (it != uid2conn_.end()) {
                if(auto conn = it->second.lock()){
					LOG_DEBUG << "Gateway: Found connection for user " << m.to() << ", sending message";
                    std::ostringstream os;
                    os << "MSG id=" << m.msg_id()
                       << " from=" << m.from()
                       << " ts=" << m.ts_ms()
                       << " text=" << m.text();
                    sendLine(conn, os.str());
                } else {
					LOG_WARN << "Gateway: Connection for user " << m.to() << " is expired";
                }
            } else {
				LOG_WARN << "Gateway: No connection found for user " << m.to();
            } });
		redis_.subscribe(ch);
	}
	else
	{
		LOG_ERROR << "redis connect failed, Deliver will not push online messages";
	}

	
}

void GatewayServer::start() { server_.start(); }

// ---------- callbacks ----------
void GatewayServer::onConnection(const TcpConnectionPtr &c)
{
	if (c->connected())
	{
		LOG_INFO << "conn up: " << c->peerAddress().toIpPort() << " name=" << c->name();
		sessions_.emplace(c->name(), Session{});
		sendLine(c, "+OK welcome. Commands: REGISTER/LOGIN/SEND/PULL");
	}
	else
	{
		LOG_INFO << "conn down: " << c->name();

		// 清理 uid->conn 映射和用户状态
		auto it = sessions_.find(c->name());
		if (it != sessions_.end() && it->second.authed)
		{
			int64_t uid = it->second.uid;
			uid2conn_.erase(uid);
			
			// 更新用户状态为离线
			mpim::LogoutReq req;
			req.set_user_id(uid);
			req.set_token(it->second.token);
			mpim::LogoutResp resp;
			MprpcController ctl;
			user_->Logout(&ctl, &req, &resp, nullptr);
			if (ctl.Failed()) {
				LOG_ERROR << "Failed to update user state to offline for uid=" << uid;
			} else {
				LOG_INFO << "User " << uid << " state updated to offline due to connection close";
			}
		}
		sessions_.erase(c->name());
		// TODO: 可在这里做心跳/TTL 的路由过期，或交由 presence 的 TTL 管理
	}
}

// 简单行分隔协议（\n），支持 CRLF
void GatewayServer::onMessage(const TcpConnectionPtr &c, Buffer *b, Timestamp)
{
	while (true)
	{
		const char *base = b->peek();
		const void *p = memchr(base, '\n', b->readableBytes());
		if (!p)
			break;
		const char *lf = static_cast<const char *>(p);
		std::string line(base, lf);
		b->retrieveUntil(lf + 1);
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		handleLine(c, line);
	}
}

void GatewayServer::handleLine(const TcpConnectionPtr &c, const std::string &line)
{
	if (line.empty())
		return;
	auto cmd_and_rest = splitOnce(line, ' ', 2);
	std::string cmd = cmd_and_rest[0];
	for (char &x : cmd)
		x = ::toupper(x);

		LOG_DEBUG << "cmd=" << cmd;
	bool okv = false;
	if (cmd == "REGISTER")
	{
		auto toks = split(cmd_and_rest.size() > 1 ? cmd_and_rest[1] : "", ' ');
		okv = handleREGISTER(c, toks);
	}
	else if (cmd == "LOGIN")
	{
		auto toks = split(cmd_and_rest.size() > 1 ? cmd_and_rest[1] : "", ' ');
		okv = handleLOGIN(c, toks);
	}
	else if (cmd == "SEND")
	{
		// SEND <toUid> <text...>
		auto toks = splitOnce(cmd_and_rest.size() > 1 ? cmd_and_rest[1] : "", ' ', 2);
		okv = handleSEND(c, toks);
	}
	else if (cmd == "PULL")
	{
		std::vector<std::string> dummy;
		okv = handlePULL(c, dummy);
	}
	else if (cmd == "LOGOUT")
	{
		std::vector<std::string> dummy;
		okv = handleLOGOUT(c, dummy);
	}
	else if (cmd == "ADDFRIEND")
	{
		auto toks = split(cmd_and_rest.size() > 1 ? cmd_and_rest[1] : "", ' ');
		okv = handleADDFRIEND(c, toks);
	}
	else if (cmd == "GETFRIENDS")
	{
		std::vector<std::string> dummy;
		okv = handleGETFRIENDS(c, dummy);
	}
	else if (cmd == "CREATEGROUP")
	{
		auto toks = splitOnce(cmd_and_rest.size() > 1 ? cmd_and_rest[1] : "", ' ', 2);
		okv = handleCREATEGROUP(c, toks);
	}
	else if (cmd == "JOINGROUP")
	{
		auto toks = split(cmd_and_rest.size() > 1 ? cmd_and_rest[1] : "", ' ');
		okv = handleJOINGROUP(c, toks);
	}
	else if (cmd == "SENDGROUP")
	{
		auto toks = splitOnce(cmd_and_rest.size() > 1 ? cmd_and_rest[1] : "", ' ', 2);
		okv = handleSENDGROUP(c, toks);
	}
	else
	{
		sendLine(c, "-ERR unknown cmd");
		return;
	}
	if (!okv)
		sendLine(c, "-ERR failed");
}

// ---------- commands ----------
bool GatewayServer::handleREGISTER(const TcpConnectionPtr &c, const std::vector<std::string> &toks)
{
	if (toks.size() < 2)
	{
		sendLine(c, "-ERR REGISTER <user> <pwd>");
		return false;
	}
	mpim::RegisterReq req;
	req.set_username(toks[0]);
	req.set_password(toks[1]);
	mpim::RegisterResp resp;
	MprpcController ctl;
	user_->Register(&ctl, &req, &resp, nullptr);
	if (ctl.Failed())
	{
		LOG_ERROR << "User.Register RPC failed: " << ctl.ErrorText();
		return false;
	}
	if (resp.result().code() != mpim::Code::Ok)
	{
		std::ostringstream os;
		os << "-ERR " << resp.result().msg();
		sendLine(c, os.str());
		return false;
	}
	// 注册成功后自动登录
	auto &sess = sessionOf(c);
	sess.authed = true;
	sess.uid = resp.user_id();
	sess.token = "tok_" + std::to_string(resp.user_id());
	
	// 连接映射（用于推送在线消息）
	uid2conn_[sess.uid] = c;
	
	// 绑定路由
	mpim::BindRouteReq br;
	br.set_user_id(sess.uid);
	br.set_gateway_id(gateway_id_);
	br.set_device("cli");
	mpim::BindRouteResp brs;
	MprpcController ctl2;
	presence_->BindRoute(&ctl2, &br, &brs, nullptr);
	
	std::ostringstream os;
	os << "+OK uid=" << resp.user_id();
	sendLine(c, os.str());
	return true;
}

bool GatewayServer::handleLOGIN(const TcpConnectionPtr &c, const std::vector<std::string> &toks)
{
	if (toks.size() < 2)
	{
		sendLine(c, "-ERR LOGIN <user> <pwd>");
		return false;
	}
	auto &sess = sessionOf(c);

	// 封装 RPC 请求参数并进行调用
	mpim::LoginReq req;
	req.set_username(toks[0]);
	req.set_password(toks[1]);
	mpim::LoginResp resp;
	MprpcController ctl;
	// RPC调用 im-user 的 Login 方法
	user_->Login(&ctl, &req, &resp, nullptr);
	// RPC 调用失败
	if (ctl.Failed() || !ok(resp.result()))
	{
		sendLine(c, "-ERR auth");
		return false;
	}
	// 登录成功，更新会话状态
	sess.authed = true;
	sess.uid = resp.user_id();
	sess.token = resp.token();

	// 连接映射（用于推送在线消息）
	uid2conn_[sess.uid] = c;
	
	// 绑定路由
	mpim::BindRouteReq br;
	br.set_user_id(sess.uid);
	br.set_gateway_id(gateway_id_);
	br.set_device("cli");
	mpim::BindRouteResp brs;
	MprpcController ctl2;
	presence_->BindRoute(&ctl2, &br, &brs, nullptr);
	if (ctl2.Failed()){
		LOG_ERROR << "Presence.BindRoute RPC failed: " << ctl2.ErrorText();
	} else if(!ok(brs.result())) {
		LOG_WARN << "Presence.BindRoute returned !OK: code=" << brs.result().code() << " msg=" << brs.result().msg();
	}

	// 构建一个成功消息，通过sendLine方法将成功消息发送给客户端
	std::ostringstream os;
	os << "+OK uid=" << sess.uid;
	sendLine(c, os.str());
	return true;
}

bool GatewayServer::handleSEND(const TcpConnectionPtr &c, const std::vector<std::string> &toks)
{
	auto &sess = sessionOf(c);
	if (!sess.authed)
	{
		sendLine(c, "-ERR not login");
		return false;
	}
	if (toks.size() < 2)
	{
		sendLine(c, "-ERR SEND <toUid> <text>");
		return false;
	}

	int64_t to = 0;
	try
	{
		to = std::stoll(toks[0]);
	}
	catch (...)
	{
		sendLine(c, "-ERR bad toUid");
		return false;
	}
	
	// 检查目标用户是否存在
	char check_sql[256];
	snprintf(check_sql, sizeof(check_sql), "SELECT 1 FROM user WHERE id=%d LIMIT 1", (int)to);
	// 这里简化处理，实际应该通过RPC调用用户服务检查
	std::string text = toks[1];

	mpim::C2CMsg m;
	m.set_from(sess.uid);
	m.set_to(to);
	m.set_text(text);
	auto nowms = std::chrono::duration_cast<std::chrono::milliseconds>(
					 std::chrono::system_clock::now().time_since_epoch())
					 .count();
	m.set_ts_ms(nowms);

	mpim::SendReq req;
	*req.mutable_msg() = m;
	mpim::SendResp resp;
	MprpcController ctl;
	message_->Send(&ctl, &req, &resp, nullptr);
	if (ctl.Failed() || !ok(resp.result()))
	{
		sendLine(c, "-ERR send");
		return false;
	}
	std::ostringstream os;
	os << "+OK msg_id=" << resp.msg_id();
	sendLine(c, os.str());
	return true;
}

bool GatewayServer::handlePULL(const TcpConnectionPtr &c, const std::vector<std::string> &)
{
	auto &sess = sessionOf(c);
	if (!sess.authed)
	{
		sendLine(c, "-ERR not login");
		return false;
	}

	mpim::PullOfflineReq req;
	req.set_user_id(sess.uid);
	mpim::PullOfflineResp resp;
	MprpcController ctl;
	message_->PullOffline(&ctl, &req, &resp, nullptr);
	if (ctl.Failed() || !ok(resp.result()))
	{
		sendLine(c, "-ERR pull");
		return false;
	}
	// 逐条下发（简单文本格式）
	for (const auto &m : resp.msg_list())
	{
		std::ostringstream os;
		os << "MSG id=" << m.msg_id()
		   << " from=" << m.from()
		   << " ts=" << m.ts_ms()
		   << " text=" << m.text();
		sendLine(c, os.str());
	}
	sendLine(c, "+OK pull_done");
	return true;
}

bool GatewayServer::handleLOGOUT(const TcpConnectionPtr &c, const std::vector<std::string> &)
{
	auto &sess = sessionOf(c);
	if (!sess.authed)
	{
		sendLine(c, "-ERR not login");
		return false;
	}

	mpim::LogoutReq req;
	req.set_user_id(sess.uid);
	req.set_token(sess.token);
	mpim::LogoutResp resp;
	MprpcController ctl;
	user_->Logout(&ctl, &req, &resp, nullptr);
	
	if (ctl.Failed() || !ok(resp.result()))
	{
		sendLine(c, "-ERR logout failed");
		return false;
	}

	// 清理会话状态
	uid2conn_.erase(sess.uid);
	sess.authed = false;
	sess.uid = 0;
	sess.token.clear();

	sendLine(c, "+OK logout success");
	return true;
}

bool GatewayServer::handleADDFRIEND(const TcpConnectionPtr &c, const std::vector<std::string> &toks)
{
	auto &sess = sessionOf(c);
	if (!sess.authed)
	{
		sendLine(c, "-ERR not login");
		return false;
	}
	if (toks.size() < 1)
	{
		sendLine(c, "-ERR ADDFRIEND <friend_id>");
		return false;
	}

	int64_t friend_id = 0;
	try
	{
		friend_id = std::stoll(toks[0]);
	}
	catch (...)
	{
		sendLine(c, "-ERR bad friend_id");
		return false;
	}

	mpim::AddFriendReq req;
	req.set_user_id(sess.uid);
	req.set_friend_id(friend_id);
	req.set_message("Hello, let's be friends!");
	mpim::AddFriendResp resp;
	MprpcController ctl;
	user_->AddFriend(&ctl, &req, &resp, nullptr);
	
	if (ctl.Failed() || !ok(resp.result()))
	{
		sendLine(c, "-ERR " + resp.result().msg());
		return false;
	}

	sendLine(c, "+OK friend added");
	return true;
}

bool GatewayServer::handleGETFRIENDS(const TcpConnectionPtr &c, const std::vector<std::string> &)
{
	auto &sess = sessionOf(c);
	if (!sess.authed)
	{
		sendLine(c, "-ERR not login");
		return false;
	}

	mpim::GetFriendsReq req;
	req.set_user_id(sess.uid);
	mpim::GetFriendsResp resp;
	MprpcController ctl;
	user_->GetFriends(&ctl, &req, &resp, nullptr);
	
	if (ctl.Failed() || !ok(resp.result()))
	{
		sendLine(c, "-ERR get friends failed");
		return false;
	}

	std::ostringstream os;
	os << "+OK friends: ";
	for (int i = 0; i < resp.friend_ids_size(); ++i)
	{
		if (i > 0) os << ",";
		os << resp.friend_ids(i);
	}
	sendLine(c, os.str());
	return true;
}

bool GatewayServer::handleCREATEGROUP(const TcpConnectionPtr &c, const std::vector<std::string> &toks)
{
	auto &sess = sessionOf(c);
	if (!sess.authed)
	{
		sendLine(c, "-ERR not login");
		return false;
	}
	if (toks.size() < 1)
	{
		sendLine(c, "-ERR CREATEGROUP <group_name> [description]");
		return false;
	}

	mpim::CreateGroupReq req;
	req.set_creator_id(sess.uid);
	req.set_group_name(toks[0]);
	req.set_description(toks.size() > 1 ? toks[1] : "");
	mpim::CreateGroupResp resp;
	MprpcController ctl;
	group_->CreateGroup(&ctl, &req, &resp, nullptr);
	
	if (ctl.Failed() || !ok(resp.result()))
	{
		sendLine(c, "-ERR " + resp.result().msg());
		return false;
	}

	std::ostringstream os;
	os << "+OK group_id=" << resp.group_id();
	sendLine(c, os.str());
	return true;
}

bool GatewayServer::handleJOINGROUP(const TcpConnectionPtr &c, const std::vector<std::string> &toks)
{
	auto &sess = sessionOf(c);
	if (!sess.authed)
	{
		sendLine(c, "-ERR not login");
		return false;
	}
	if (toks.size() < 1)
	{
		sendLine(c, "-ERR JOINGROUP <group_id>");
		return false;
	}

	int64_t group_id = 0;
	try
	{
		group_id = std::stoll(toks[0]);
	}
	catch (...)
	{
		sendLine(c, "-ERR bad group_id");
		return false;
	}

	mpim::JoinGroupReq req;
	req.set_user_id(sess.uid);
	req.set_group_id(group_id);
	mpim::JoinGroupResp resp;
	MprpcController ctl;
	group_->JoinGroup(&ctl, &req, &resp, nullptr);
	
	if (ctl.Failed() || !ok(resp.result()))
	{
		sendLine(c, "-ERR " + resp.result().msg());
		return false;
	}

	sendLine(c, "+OK joined group");
	return true;
}

bool GatewayServer::handleSENDGROUP(const TcpConnectionPtr &c, const std::vector<std::string> &toks)
{
	auto &sess = sessionOf(c);
	if (!sess.authed)
	{
		sendLine(c, "-ERR not login");
		return false;
	}
	if (toks.size() < 2)
	{
		sendLine(c, "-ERR SENDGROUP <group_id> <text>");
		return false;
	}

	int64_t group_id = 0;
	try
	{
		group_id = std::stoll(toks[0]);
	}
	catch (...)
	{
		sendLine(c, "-ERR bad group_id");
		return false;
	}
	std::string text = toks[1];

	mpim::GroupMsg m;
	m.set_from(sess.uid);
	m.set_group_id(group_id);
	m.set_text(text);
	auto nowms = std::chrono::duration_cast<std::chrono::milliseconds>(
					 std::chrono::system_clock::now().time_since_epoch())
					 .count();
	m.set_ts_ms(nowms);

	mpim::SendGroupReq req;
	*req.mutable_msg() = m;
	mpim::SendGroupResp resp;
	MprpcController ctl;
	message_->SendGroup(&ctl, &req, &resp, nullptr);
	if (ctl.Failed() || !ok(resp.result()))
	{
		sendLine(c, "-ERR send group message");
		return false;
	}
	std::ostringstream os;
	os << "+OK group_msg_id=" << resp.msg_id();
	sendLine(c, os.str());
	return true;
}
