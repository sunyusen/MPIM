#include "presence_service.h"
#include "logger/logger.h"
#include "logger/log_init.h"
#include <sstream>
#include <thread>
#include <chrono>

using mpim::Code;

static inline std::string RouteKey(int64_t uid)
{
	return "route:" + std::to_string(uid);
}

PresenceServiceImpl::PresenceServiceImpl()
{
	// Try to connect to Redis with retry logic
	int retry_count = 0;
	const int max_retries = 3;
	
	while (retry_count < max_retries) {
		if (r_.connect()) {
			std::cout << "PresenceServiceImpl: Redis connected successfully" << std::endl;
			redis_connected_ = true;
			return;
		}
		
		retry_count++;
		std::cout << "PresenceServiceImpl: Redis connection attempt " << retry_count << " failed, retrying..." << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	
	std::cout << "PresenceServiceImpl: Redis connection failed after " << max_retries << " attempts! Service will start but message delivery will be disabled." << std::endl;
	redis_connected_ = false;
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
	
	// Check if Redis is connected, try to reconnect if needed
	if (!redis_connected_) {
		std::cout << "Redis not connected, attempting to reconnect..." << std::endl;
		if (r_.connect()) {
			redis_connected_ = true;
			std::cout << "Redis reconnected successfully" << std::endl;
		} else {
			std::cout << "Redis reconnection failed, cannot bind route for user " << req->user_id() << std::endl;
			resp->mutable_result()->set_code(Code::INTERNAL);
			resp->mutable_result()->set_msg("redis not connected");
			if (done) done->Run();
			return;
		}
	}
	
	// 设置键值对并指定过期时间
	try {
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
	} catch (const std::exception& e) {
		std::cout << "Exception in BindRoute: " << e.what() << std::endl;
		resp->mutable_result()->set_code(Code::INTERNAL);
		resp->mutable_result()->set_msg("redis operation failed");
	}
	if (done)
		done->Run();
}

void PresenceServiceImpl::QueryRoute(google::protobuf::RpcController *,
									 const mpim::QueryRouteReq *req,
									 mpim::QueryRouteResp *resp,
									 google::protobuf::Closure *done)
{
	std::string route_key = RouteKey(req->user_id());
	std::cout << "PresenceService::QueryRoute: user_id=" << req->user_id() 
			  << " route_key=" << route_key << std::endl;
	
	// Check if Redis is connected, try to reconnect if needed
	if (!redis_connected_) {
		std::cout << "Redis not connected, attempting to reconnect..." << std::endl;
		if (r_.connect()) {
			redis_connected_ = true;
			std::cout << "Redis reconnected successfully" << std::endl;
		} else {
			std::cout << "Redis reconnection failed, cannot query route for user " << req->user_id() << std::endl;
			resp->mutable_result()->set_code(Code::INTERNAL);
			resp->mutable_result()->set_msg("redis not connected");
			if (done) done->Run();
			return;
		}
	}
	
	// 根据用户ID获取对应的网关ID
	try {
		auto gw = r_.get(route_key);
		if (!gw.empty())
		{
			std::cout << "Found route for user " << req->user_id() << " gateway=" << gw << std::endl;
			resp->set_gateway_id(gw);
			resp->mutable_result()->set_code(Code::Ok);
		}
		else
		{
			std::cout << "No route found for user " << req->user_id() << std::endl;
			resp->mutable_result()->set_code(Code::NOT_FOUND);
			resp->mutable_result()->set_msg("no route");
		}
	} catch (const std::exception& e) {
		std::cout << "Exception in QueryRoute: " << e.what() << std::endl;
		resp->mutable_result()->set_code(Code::INTERNAL);
		resp->mutable_result()->set_msg("redis operation failed");
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
	if (!redis_connected_) {
		std::cout << "PresenceService::Deliver: Redis not connected, cannot deliver message" << std::endl;
		resp->mutable_result()->set_code(Code::INTERNAL);
		resp->mutable_result()->set_msg("redis not connected");
		if (done) done->Run();
		return;
	}
	
	std::string route_key = RouteKey(req->to());
	std::cout << "PresenceService::Deliver: to=" << req->to() 
			  << " route_key=" << route_key << std::endl;
	
	// 在redis上查询接受者的路由信息
	auto gw = r_.get(route_key);
	if (gw.empty())		// 目标用户不在线
	{
		std::cout << "Receiver " << req->to() << " is offline" << std::endl;
		resp->mutable_result()->set_code(Code::NOT_FOUND);
		resp->mutable_result()->set_msg("receiver offline");
		if (done)
			done->Run();
		return;
	}

	std::cout << "Found gateway " << gw << " for user " << req->to() << std::endl;
	
	// 根据获取到的gw，将消息发布到对应网关的通道上
	int ch = GwChannelOf(gw);		// 获取网关对应的通道号
	std::cout << "Publishing to channel " << ch << " for gateway " << gw << std::endl;
	
	if (!r_.publish(ch, std::string{reinterpret_cast<const char *>(req->payload().data()),
									req->payload().size()}))
	{
		std::cout << "Failed to publish message to channel " << ch << std::endl;
		resp->mutable_result()->set_code(Code::INTERNAL);
		resp->mutable_result()->set_msg("redis publish failed");
	}
	else
	{
		std::cout << "Message published successfully to channel " << ch << std::endl;
		resp->mutable_result()->set_code(Code::Ok);
	}
	if (done)
		done->Run();
}
