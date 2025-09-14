#pragma once

#include "google/protobuf/service.h"
#include <memory>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <functional>
#include <google/protobuf/descriptor.h>
#include <string>
#include <unordered_map>

class RpcProvider
{
public:
	// 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
	void NotifyService(google::protobuf::Service *service);
	
	// 支持智能指针的重载版本
	template<typename T>
	void NotifyService(std::unique_ptr<T> service) {
		// 将智能指针的所有权转移给RpcProvider
		NotifyService(service.release());
	}

	// 启动rpc服务节点，开始提供rpc远程网络调用服务
	void Run();

private:
	// 组合了EventLoop
	muduo::net::EventLoop m_eventLoop;

	// service服务类型信息
	struct ServiceInfo
	{
		google::protobuf::Service *m_service;													 // 保存服务对象
		std::unique_ptr<google::protobuf::Service> m_service_owner; // 拥有服务对象的所有权
		std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> m_methodMap; // 保存服务方法
	};
	// 存储注册成功的服务对象和其服务方法的所有信息
	std::unordered_map<std::string, ServiceInfo> m_serviceMap;

	// 新的socket连接回调
	void OnConnection(const muduo::net::TcpConnectionPtr &); // 使用完整的类型定义
	// 已建立连接用户的读写事件回调
	void OnMessage(const muduo::net::TcpConnectionPtr &, muduo::net::Buffer *, muduo::Timestamp); // 修正类型定义
	// Closure的回调操作,用于序列化rpc的响应和网络发送
	void SendRpcResponse(const muduo::net::TcpConnectionPtr &, google::protobuf::Message *);
};