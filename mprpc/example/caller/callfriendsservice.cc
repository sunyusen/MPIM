#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"

int main(int argc, char **argv)
{
	// 整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定要先调用框架的初始化函数（只初始化一次）
	MprpcApplication::Init(argc, argv);

	fixbug::FriendServiceRpc_Stub stub(new MprpcChannel()); // 创建FriendServiceRpc_Stub对象，用于调用远程的FriendService服务，它是一个代理对象
	fixbug::GetFriendsListRequest request;
	request.set_userid(10001);				 // 设置请求参数userid
	fixbug::GetFriendsListResponse response; // rpc方法的响应
	MprpcController controller;
	stub.GetFriendsList(&controller, &request, &response, nullptr); // 发起rpc方法的调用 同步rpc的调用过程
	// 一次rpc调用完成，读调用的结果
	if (controller.Failed())
	{
		std::cout << "rpc GetFriendsList failed: " << controller.ErrorText() << std::endl;
		return -1;
	}
	else
	{
		if (0 == response.result().errcode())
		{
			std::cout << "rpc GetFriendsList response: " << std::endl;
			for (const std::string &name : response.friends())
			{
				std::cout << name << std::endl; // 输出好友列表
			}
		}
		else
		{
			std::cout << "rpc GetFriendsList response error: " << response.result().errmsg() << std::endl;
		}
	}

	return 0;
}