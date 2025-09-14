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
			LOG_INFO << "Redis connected successfully";
			redis_connected_ = true;
			return;
		}
		
		retry_count++;
		LOG_WARN << "Redis connection attempt " << retry_count << " failed, retrying...";
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	
	LOG_ERROR << "Redis connection failed after " << max_retries << " attempts! Service will start but message delivery will be disabled.";
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
	LOG_DEBUG << "BindRoute: user_id=" << req->user_id() << 
			  " gateway_id=" << req->gateway_id() << 
			  " route_key=" << route_key;
	
	// Check if Redis is connected, try to reconnect if needed
	if (!redis_connected_) {
		LOG_WARN << "Redis not connected, attempting to reconnect...";
		if (r_.connect()) {
			redis_connected_ = true;
			LOG_INFO << "Redis reconnected successfully";
		} else {
			LOG_ERROR << "Redis reconnection failed, cannot bind route for user " << req->user_id();
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
			LOG_INFO << "Route bound successfully for user " << req->user_id();
			resp->mutable_result()->set_code(Code::Ok);
		}
		else
		{
			LOG_ERROR("Failed to bind route for user " + std::to_string(req->user_id()));
			resp->mutable_result()->set_code(Code::INTERNAL);
			resp->mutable_result()->set_msg("redis setex failed");
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Exception in BindRoute: " + std::string(e.what()));
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
	LOG_DEBUG("QueryRoute: user_id=" + std::to_string(req->user_id()) + 
			  " route_key=" + route_key);
	
	// Check if Redis is connected, try to reconnect if needed
	if (!redis_connected_) {
		LOG_WARN << "Redis not connected, attempting to reconnect...";
		if (r_.connect()) {
			redis_connected_ = true;
			LOG_INFO << "Redis reconnected successfully";
		} else {
			LOG_ERROR("Redis reconnection failed, cannot query route for user " + std::to_string(req->user_id()));
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
			LOG_INFO("Found route for user " + std::to_string(req->user_id()) + " gateway=" + gw);
			resp->set_gateway_id(gw);
			resp->mutable_result()->set_code(Code::Ok);
		}
		else
		{
			LOG_WARN("No route found for user " + std::to_string(req->user_id()));
			resp->mutable_result()->set_code(Code::NOT_FOUND);
			resp->mutable_result()->set_msg("no route");
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Exception in QueryRoute: " + std::string(e.what()));
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
		LOG_ERROR("Deliver: Redis not connected, cannot deliver message");
		resp->mutable_result()->set_code(Code::INTERNAL);
		resp->mutable_result()->set_msg("redis not connected");
		if (done) done->Run();
		return;
	}
	
	std::string route_key = RouteKey(req->to());
	LOG_DEBUG("Deliver: to=" + std::to_string(req->to()) + 
			  " route_key=" + route_key);
	
	// 在redis上查询接受者的路由信息
	auto gw = r_.get(route_key);
	if (gw.empty())		// 目标用户不在线
	{
		LOG_WARN("Receiver " + std::to_string(req->to()) + " is offline");
		resp->mutable_result()->set_code(Code::NOT_FOUND);
		resp->mutable_result()->set_msg("receiver offline");
		if (done)
			done->Run();
		return;
	}

	LOG_INFO("Found gateway " + gw + " for user " + std::to_string(req->to()));
	
	// 根据获取到的gw，将消息发布到对应网关的通道上
	int ch = GwChannelOf(gw);		// 获取网关对应的通道号
	LOG_DEBUG("Publishing to channel " + std::to_string(ch) + " for gateway " + gw);
	
	if (!r_.publish(ch, std::string{reinterpret_cast<const char *>(req->payload().data()),
									req->payload().size()}))
	{
		LOG_ERROR("Failed to publish message to channel " + std::to_string(ch));
		resp->mutable_result()->set_code(Code::INTERNAL);
		resp->mutable_result()->set_msg("redis publish failed");
	}
	else
	{
		LOG_INFO("Message published successfully to channel " + std::to_string(ch));
		resp->mutable_result()->set_code(Code::Ok);
	}
	if (done)
		done->Run();
}
