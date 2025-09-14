#include "presence_service.h"
#include <sstream>

using mpim::Code;

static inline std::string RouteKey(int64_t uid)
{
	return "route:" + std::to_string(uid);
}

PresenceServiceImpl::PresenceServiceImpl()
{
	if (!r_.connect()) {
		std::cout << "Redis connection failed in PresenceServiceImpl!" << std::endl;
		exit(EXIT_FAILURE);
	}
	std::cout << "PresenceServiceImpl: Redis connected successfully" << std::endl;
}

// 将网关ID映射到一个整形通道号， 简单hash
int PresenceServiceImpl::GwChannelOf(const std::string &gw)
{
	// 简单稳定映射：10000 ~ 59999
	return 10000 + static_cast<int>(std::hash<std::string>{}(gw) % 50000);
}

void PresenceServiceImpl::BindRoute(google::protobuf::RpcController *,
									const mpim::BindRouteReq *req,
									mpim::BindRouteResp *resp,
									google::protobuf::Closure *done)
{
	std::string route_key = RouteKey(req->user_id());
	std::cout << "PresenceService::BindRoute: user_id=" << req->user_id() 
			  << " gateway_id=" << req->gateway_id() 
			  << " route_key=" << route_key << std::endl;
	
	// 设置键值对并指定过期时间
	if (r_.setex(route_key, 120, req->gateway_id()))
	{
		std::cout << "Route bound successfully for user " << req->user_id() << std::endl;
		resp->mutable_result()->set_code(Code::Ok);
	}
	else
	{
		std::cout << "Failed to bind route for user " << req->user_id() << std::endl;
		resp->mutable_result()->set_code(Code::INTERNAL);
		resp->mutable_result()->set_msg("redis setex failed");
	}
	if (done)
		done->Run();
}

void PresenceServiceImpl::QueryRoute(google::protobuf::RpcController *,
									 const mpim::QueryRouteReq *req,
									 mpim::QueryRouteResp *resp,
									 google::protobuf::Closure *done)
{
	// 根据用户ID获取对应的网关ID
	auto gw = r_.get(RouteKey(req->user_id()));
	if (!gw.empty())
	{
		resp->set_gateway_id(gw);
		resp->mutable_result()->set_code(Code::Ok);
	}
	else
	{
		resp->mutable_result()->set_code(Code::NOT_FOUND);
		resp->mutable_result()->set_msg("no route");
	}
	if (done)
		done->Run();
}

// 
void PresenceServiceImpl::Deliver(google::protobuf::RpcController *,
								  const mpim::DeliveReq *req,
								  mpim::DeliveResp *resp,
								  google::protobuf::Closure *done)
{
	// 在redis上查询接受者的路由信息
	auto gw = r_.get(RouteKey(req->to()));
	if (gw.empty())		// 目标用户不在线
	{
		resp->mutable_result()->set_code(Code::NOT_FOUND);
		resp->mutable_result()->set_msg("receiver offline");
		if (done)
			done->Run();
		return;
	}

	// 根据获取到的gw，将消息发布到对应网关的通道上
	int ch = GwChannelOf(gw);		// 获取网关对应的通道号
	if (!r_.publish(ch, std::string{reinterpret_cast<const char *>(req->payload().data()),
									req->payload().size()}))
	{
		resp->mutable_result()->set_code(Code::INTERNAL);
		resp->mutable_result()->set_msg("redis publish failed");
	}
	else
	{
		resp->mutable_result()->set_code(Code::Ok);
	}
	if (done)
		done->Run();
}
