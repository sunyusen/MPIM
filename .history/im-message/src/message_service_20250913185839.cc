#include "message_service.h"

MessageServiceImpl::MessageServiceImpl()
{
	// 初始化Presence服务RPC通道
	ch_presence_.reset(new MprpcChannel());
	presence_.reset(new mpim::PresenceService_Stub(ch_presence_.get()));
	
	// 初始化离线消息存储
	offline_ = OfflineModel();
	
	std::cout << "MessageServiceImpl: Initialized with Presence RPC channel and OfflineModel" << std::endl;
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
	presence_->QueryRoute(&ctl1, &qr, &qrs, nullptr);

	// demo: 生成一个递增 msg_id（真实可用库自增或雪花）
	static std::atomic<long long> g_id{1};
	resp->set_msg_id(g_id++);

	std::cout << "MessageService::Send: from=" << m.from() << " to=" << m.to() << " text=" << m.text() << std::endl;

	// 2) 在线 -> 让 presence 代投（Redis 发给对应网关）
	if (!ctl1.Failed() && qrs.result().code() == mpim::Code::Ok && !qrs.gateway_id().empty())
	{
		std::cout << "User " << m.to() << " is online, gateway=" << qrs.gateway_id() << std::endl;
		mpim::DeliveReq dr;
		dr.set_to(m.to());
		std::string bytes = m.SerializeAsString();
		dr.set_payload(bytes);
		mpim::DeliveResp dresp;
		MprpcController ctl2;
		presence_->Deliver(&ctl2, &dr, &dresp, nullptr);
		if (!ctl2.Failed() && dresp.result().code() == mpim::Code::Ok)
		{
			std::cout << "Message delivered successfully to user " << m.to() << std::endl;
			resp->mutable_result()->set_code(mpim::Code::Ok);
			resp->mutable_result()->set_msg("delivered");
			if (done)
				done->Run();
			return;
		}
		else
		{
			std::cout << "Message delivery failed: " << ctl2.ErrorText() << std::endl;
		}
	}
	else
	{
		std::cout << "User " << m.to() << " is offline or route not found" << std::endl;
	}

	// 3) 不在线/投递失败 -> 离线落库
	std::cout << "Storing message offline for user " << m.to() << std::endl;
	if (offline_.insert(m.to(), m.SerializeAsString()))
	{
		std::cout << "Message stored offline successfully for user " << m.to() << std::endl;
	}
	else
	{
		std::cout << "Failed to store message offline for user " << m.to() << std::endl;
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
	// 1）从离线消息数据库中查询接收者的离线消息
	auto bins = offline_.query(req->user_id());
	// 2）将离线消息反序列化并添加到响应中
	for (auto &b : bins)
	{
		mpim::C2CMsg *msg = resp->add_msg_list();
		msg->ParseFromArray(b.data(), (int)b.size());
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
