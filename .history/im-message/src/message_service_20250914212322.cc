#include "message_service.h"
#include "logger/logger.h"
#include "logger/log_init.h"

MessageServiceImpl::MessageServiceImpl()
{
	// 初始化离线消息存储
	offline_ = OfflineModel();
	
	// 初始化Presence服务RPC通道
	try {
		ch_presence_.reset(new MprpcChannel());
		presence_.reset(new mpim::PresenceService_Stub(ch_presence_.get()));
		
			LOG_INFO << "MessageServiceImpl: Initialized with Presence RPC channel and OfflineModel";
		
		// 检查初始化是否成功
		if (!ch_presence_) {
				LOG_ERROR << "MessageServiceImpl: ERROR - Failed to create Presence RPC channel!";
		}
		if (!presence_) {
				LOG_ERROR << "MessageServiceImpl: ERROR - Failed to create Presence RPC stub!";
		}
	} catch (const std::exception& e) {
		LOG_ERROR << "MessageServiceImpl: Exception during initialization: " << e.what();
		ch_presence_.reset();
		presence_.reset();
	}
}

// 
void MessageServiceImpl::Send(google::protobuf::RpcController *,
							  const mpim::SendReq *req,
							  mpim::SendResp *resp,
							  google::protobuf::Closure *done)
{
	const auto &m = req->msg();
	// 1) 调用Presence服务的QueryRoute方法，查询接收者的路由信息
	mpim::QueryRouteReq qr;
	qr.set_user_id(m.to());
	mpim::QueryRouteResp qrs;
	MprpcController ctl1;
	LOG_INFO("MessageService::Send: Calling Presence QueryRoute for user " + std::to_string(m.to()));
	
	// 检查RPC通道是否有效
	if (!ch_presence_ || !presence_) {
		LOG_ERROR("MessageService::Send: Presence RPC channel or stub is null!");
		ctl1.SetFailed("Presence RPC channel not available");
	} else {
		LOG_DEBUG("MessageService::Send: Presence RPC channel is valid");
		presence_->QueryRoute(&ctl1, &qr, &qrs, nullptr);
	}
	
	if (ctl1.Failed()) {
		LOG_ERROR("MessageService::Send: Presence QueryRoute RPC failed: " + ctl1.ErrorText());
		LOG_ERROR("MessageService::Send: RPC error details - errno: " + std::to_string(errno));
	} else {
		LOG_INFO("MessageService::Send: Presence QueryRoute result code=" + std::to_string(qrs.result().code()) + " gateway_id=" + qrs.gateway_id());
	}

	// demo: 生成一个递增 msg_id（真实可用库自增或雪花）
	static std::atomic<long long> g_id{1};
	resp->set_msg_id(g_id++);

	LOG_INFO("MessageService::Send: from=" + std::to_string(m.from()) + " to=" + std::to_string(m.to()) + " text=" + m.text());

	// 2) 在线 -> 让 presence 代投（Redis 发给对应网关）
	if (!ctl1.Failed() && qrs.result().code() == mpim::Code::Ok && !qrs.gateway_id().empty())
	{
		LOG_INFO("User " + std::to_string(m.to()) + " is online, gateway=" + qrs.gateway_id());
		LOG_INFO("MessageService::Send: QueryRoute successful, proceeding with delivery");
		mpim::DeliveReq dr;
		dr.set_to(m.to());
		std::string bytes = m.SerializeAsString();
		dr.set_payload(bytes);
		mpim::DeliveResp dresp;
		MprpcController ctl2;
		LOG_INFO("MessageService::Send: Calling Presence Deliver for user " + std::to_string(m.to()));
		
		if (presence_) {
			presence_->Deliver(&ctl2, &dr, &dresp, nullptr);
		} else {
			ctl2.SetFailed("Presence RPC stub not available");
		}
		
		if (ctl2.Failed()) {
			LOG_ERROR("MessageService::Send: Presence Deliver RPC failed: " + ctl2.ErrorText());
		} else {
			LOG_INFO("MessageService::Send: Presence Deliver result code=" + std::to_string(dresp.result().code()) + " msg=" + dresp.result().msg());
		}
		
		if (!ctl2.Failed() && dresp.result().code() == mpim::Code::Ok)
		{
			LOG_INFO("Message delivered successfully to user " + std::to_string(m.to()));
			resp->mutable_result()->set_code(mpim::Code::Ok);
			resp->mutable_result()->set_msg("delivered");
			if (done)
				done->Run();
			return; // 在线投递成功，直接返回，不执行离线存储
		}
		else
		{
			LOG_ERROR("Message delivery failed: " + ctl2.ErrorText() + " result_code=" + std::to_string(dresp.result().code()));
			// 在线投递失败，继续执行离线存储
		}
	}
	else
	{
		LOG_WARN("User " + std::to_string(m.to()) + " is offline or route not found");
	}

	// 3) 不在线/投递失败 -> 离线落库
	LOG_INFO("Storing message offline for user " + std::to_string(m.to()));
	if (offline_.insert(m.to(), m.SerializeAsString()))
	{
		LOG_INFO("Message stored offline successfully for user " + std::to_string(m.to()));
	}
	else
	{
		LOG_ERROR("Failed to store message offline for user " + std::to_string(m.to()));
		// 即使存储失败，也返回成功，避免消息丢失
	}
	resp->mutable_result()->set_code(mpim::Code::Ok);
	resp->mutable_result()->set_msg("queued");
	if (done)
		done->Run();
}

// 处理拉取离线消息的请求
void MessageServiceImpl::PullOffline(google::protobuf::RpcController *,
								 const mpim::PullOfflineReq *req,
								 mpim::PullOfflineResp *resp,
								 google::protobuf::Closure *done)
{
	LOG_INFO("MessageService::PullOffline: user_id=" + std::to_string(req->user_id()));
	
	// 1）从离线消息数据库中查询接收者的离线消息
	auto bins = offline_.query(req->user_id());
	LOG_INFO("MessageService::PullOffline: found " + std::to_string(bins.size()) + " offline messages");
	
	// 2）将离线消息反序列化并添加到响应中
	for (auto &b : bins)
	{
		mpim::C2CMsg *msg = resp->add_msg_list();
		if (!msg->ParseFromArray(b.data(), (int)b.size())) {
			LOG_ERROR("MessageService::PullOffline: Failed to parse message, size=" + std::to_string(b.size()));
		} else {
			LOG_DEBUG("MessageService::PullOffline: Parsed message from=" + std::to_string(msg->from()) + " to=" + std::to_string(msg->to()) + " text=" + msg->text());
		}
	}
	
	// 3）拉取群组离线消息
	auto group_bins = offline_.queryGroupMessages(0); // 简化处理，拉取所有群组消息
	for (auto &b : group_bins)
	{
		mpim::GroupMsg *msg = resp->add_group_msg_list();
		msg->ParseFromArray(b.data(), (int)b.size());
	}
	
	// 4）删除离线消息
	offline_.remove(req->user_id());
	offline_.removeGroupMessages(0);
	resp->mutable_result()->set_code(mpim::Code::Ok);
	resp->mutable_result()->set_msg("ok");
	if (done)
		done->Run();
}

void MessageServiceImpl::Ack(google::protobuf::RpcController *,
							 const mpim::AckReq *, mpim::AckResp *resp,
							 google::protobuf::Closure *done)
{
	// 确认消息已被接收
	resp->mutable_result()->set_code(mpim::Code::Ok);
	// 直接返回成功状态（可更新数据库或删除消息）
	if (done)
		done->Run();
}

void MessageServiceImpl::SendGroup(google::protobuf::RpcController *,
								  const mpim::SendGroupReq *req,
								  mpim::SendGroupResp *resp,
								  google::protobuf::Closure *done)
{
	const auto &m = req->msg();
	
	// demo: 生成一个递增 msg_id
	static std::atomic<long long> g_id{1};
	resp->set_msg_id(g_id++);
	
	// 这里需要获取群组成员列表，然后向每个在线成员投递消息
	// 简化实现：直接存储为离线消息，由客户端拉取
	// 实际项目中应该查询群组成员，然后逐个投递
	
	// 存储群组消息到离线存储
	offline_.insertGroupMessage(m.group_id(), m.SerializeAsString());
	
	resp->mutable_result()->set_code(mpim::Code::Ok);
	resp->mutable_result()->set_msg("group message queued");
	if (done)
		done->Run();
}
