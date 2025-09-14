#include <iostream>
#include "mprpcapplication.h"
#include "mprpc/example/proto/user.pb.h"

int main(int argc, char **argv)
{
	// 整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定要先调用框架的初始化函数（只初始化一次）
	MprpcApplication::Init(argc, argv);

	// 创建UserServiceRpc_Stub对象，用于调用远程的UserService服务，它是一个代理对象
	// UserServiceRpc_Stub是由protoc编译器根据user.proto文件生成的，这是自己写的方法
	// new MprpcChannel():创建一个MprpcChannel对象，作为RPC通信的信道。
	fixbug::UserServiceRpc_Stub stub(new MprpcChannel());	//MprpcChannel是处理RPC调用的底层逻辑
	fixbug::LoginRequest request;
	request.set_name("zhangsan");
	request.set_pwd("123456");
	// rpc方法的响应
	fixbug::LoginResponse response;
	// 发起rpc方法的调用 同步rpc的调用过程	MprpcChannel::callmethod
	stub.Login(nullptr, &request, &response, nullptr);	//RpcChannel->RpcChannel::callMethod	集中来做所有rpc方法调用的参数序列化和网络发送

	// 一次rpc调用完成，读调用的结果
	if(0 == response.result().errcode())
	{
		std::cout << "rpc Login response: " << response.success() << std::endl;
	}
	else
	{
		std::cout << "rpc login response error: " << response.result().errmsg() << std::endl;
	}

	// 演示调用远程发布的rpc方法Register
	fixbug::RegisterRequest register_request;
	register_request.set_id(10001);
	register_request.set_name("lisi");
	register_request.set_pwd("123456");
	fixbug::RegisterResponse register_response;

	// 以同步的方式发起rpc调用请求，等待返回结果
	stub.Register(nullptr, &register_request, &register_response, nullptr);

	if(0 == register_response.result().errcode())
	{
		std::cout << "rpc Register response: " << register_response.success() << std::endl;
	}
	else
	{
		std::cout << "rpc Register response error: " << register_response.result().errmsg() << std::endl;
	}

	return 0;
}