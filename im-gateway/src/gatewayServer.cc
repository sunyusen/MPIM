#include "gatewayServer.h"
#include <chrono>
#include <sstream>
#include <functional>

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
	// 你当前 Redis 封装的 pub/sub 是 int 通道；这里把 "gw:<id>" 做个稳定映射
	// 必须和 im-presence 使用的算法一致（若你在 presence 里直接用了固定整数，也请改成一致）。
	return static_cast<int>(std::hash<std::string>{}("gw:" + gateway_id_) & 0x7fffffff);
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

	user_.reset(new mpim::UserService_Stub(ch_user_.get()));
	presence_.reset(new mpim::PresenceService_Stub(ch_presence_.get()));
	message_.reset(new mpim::MessageService_Stub(ch_message_.get()));

	// 订阅网关通道，接收 presence.Deliver 投递的 C2C 消息（序列化后的 C2CMsg）
	if (redis_.connect())
	{
		int ch = gatewayChannel();
		redis_.init_notify_handler([this](int /*channel*/, std::string payload)
								   {
            mpim::C2CMsg m;
            if(!m.ParseFromString(payload)) {
                LOG_WARN << "bad payload for C2CMsg";
                return;
            }
            auto it = uid2conn_.find(m.to());
            if (it != uid2conn_.end()) {
                if(auto conn = it->second.lock()){
                    std::ostringstream os;
                    os << "MSG id=" << m.msg_id()
                       << " from=" << m.from()
                       << " ts=" << m.ts_ms()
                       << " text=" << m.text();
                    sendLine(conn, os.str());
                }
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
		sendLine(c, "+OK welcome. Commands: LOGIN/SEND/PULL");
	}
	else
	{
		LOG_INFO << "conn down: " << c->name();

		// 清理 uid->conn 映射
		auto it = sessions_.find(c->name());
		if (it != sessions_.end() && it->second.authed)
		{
			uid2conn_.erase(it->second.uid);
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

	bool okv = false;
	if (cmd == "LOGIN")
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
	else
	{
		sendLine(c, "-ERR unknown cmd");
		return;
	}
	if (!okv)
		sendLine(c, "-ERR failed");
}

// ---------- commands ----------
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
	// RPC 调用 presence 的 BindRoute 方法
	presence_->BindRoute(&ctl2, &br, &brs, nullptr);
	// // 检查路由绑定是否成功
	// if (ctl2.Failed() || !ok(brs.result()))
	// {
	// 	LOG_WARN << "BindRoute failed for uid=" << sess.uid;
	// }
	if (ctl2.Failed()){
		LOG_ERROR << "Presence.BindRoute RPC failed: " << ctl2.ErrorText();
	} else if(!ok(brs.result())) {
		LOG_WARN << "Presence.BindRoute returned !OK: code=" << resp.result().code()
              << " msg=" << resp.result().msg();
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
