#include "message_service.h"

MessageServiceImpl::MessageServiceImpl()
{
	ch_presence_.reset(new MprpcChannel());
	presence_.reset(new mpim::PresenceService_Stub(ch_presence_.get()));
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

	// 2) 在线 -> 让 presence 代投（Redis 发给对应网关）
	if (!ctl1.Failed() && qrs.result().code() == mpim::Code::Ok && !qrs.gateway_id().empty())
	{
		mpim::DeliveReq dr;
		dr.set_to(m.to());
		std::string bytes = m.SerializeAsString();
		dr.set_payload(bytes);
		mpim::DeliveResp dresp;
		MprpcController ctl2;
		presence_->Deliver(&ctl2, &dr, &dresp, nullptr);
		if (!ctl2.Failed() && dresp.result().code() == mpim::Code::Ok)
		{
			resp->mutable_result()->set_code(mpim::Code::Ok);
			resp->mutable_result()->set_msg("delivered");
			if (done)
				done->Run();
			return;
		}
	}

	// 3) 不在线/投递失败 -> 离线落库
	offline_.insert(m.to(), m.SerializeAsString());
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
	// 3）删除离线消息
	offline_.remove(req->user_id());
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
