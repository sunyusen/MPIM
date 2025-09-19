#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <string>
#include <netinet/in.h>


class MprpcChannel: public google::protobuf::RpcChannel
{
public:
	// 所有通过stub代理对象调用的rpc方法，都走到这里了，统一做rpc方法调用的数据序列化和网络发送
	void CallMethod(const google::protobuf::MethodDescriptor* method,
					gooyi'donggle::protobuf::RpcController* controller,
					const google::protobuf::Message* request,
					google::protobuf::Message* response,
					google::protobuf::Closure* done);

private:
	// 将 method/request 序列化为 header(len, service, method) + body 的帧
	bool buildRequestFrame(const google::protobuf::MethodDescriptor* method,
							 const google::protobuf::Message* request,
							 std::string& out,
							 google::protobuf::RpcController* controller);

	// 解析 ZK 地址并做简单 TTL 缓存
	bool resolveEndpoint(const std::string& service,
						 const std::string& method,
						 struct sockaddr_in& out,
						 google::protobuf::RpcController* controller);

	// 连接池：按 key 复用/归还连接（必要时拨号）
	int getConnection(const std::string& key, const struct sockaddr_in& addr);
	void returnConnection(const std::string& key, int fd);

	// 阻塞发送/接收（带错误上报）
	bool sendAll(int fd, const char* data, size_t len, google::protobuf::RpcController* controller);
	bool recvResponse(int fd, google::protobuf::Message* response, google::protobuf::RpcController* controller);
};