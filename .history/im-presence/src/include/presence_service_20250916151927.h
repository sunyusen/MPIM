#pragma once
#include "presence.pb.h"
#include "rpcprovider.h"
#include "redis_client.h"
#include "cache_manager.h"
#include "message_queue.h"
#include "route_cache.h"
#include <memory>

/*
PresenceServiceImpl是一个路由管理服务的实现类，主要负责
1. 绑定用户的路由信息（用户ID与网关ID的映射）
2. 查询用户的路由信息（根据用户ID获取网关ID）
3. 分发消息（将消息从一个用户发送到另一个用户）
*/
class PresenceServiceImpl : public mpim::PresenceService
{
public:
	PresenceServiceImpl();
	// 在redis上 绑定用户的路由信息
	// 将用户ID和网关ID绑定起来，记录用户当前连接的网关信息
	void BindRoute(::google::protobuf::RpcController *,
				   const ::mpim::BindRouteReq *,
				   ::mpim::BindRouteResp *,
				   ::google::protobuf::Closure *) override;
	// 在redis上，查询用户的路由信息
	// 根据用户ID查询用户当前连接的网关ID
	void QueryRoute(::google::protobuf::RpcController *,
					const ::mpim::QueryRouteReq *,
					::mpim::QueryRouteResp *,
					::google::protobuf::Closure *) override;
	// 分发消息
	// 将消息从一个用户发送到另一个用户
	void Deliver(::google::protobuf::RpcController *,
				 const ::mpim::DeliveReq *,
				 ::mpim::DeliveResp *,
				 ::google::protobuf::Closure *) override;

private:
	// 将网关ID映射到一个整形通道号
	// 与redis接口匹配，用于在Redis中存储或查询路由信息
	static int GwChannelOf(const std::string &gateway_id);
	mpim::redis::CacheManager cache_manager_;
	// Redis消息队列，用于消息投递
	mpim::redis::MessageQueue message_queue_;
	bool redis_connected_ = false;
};
