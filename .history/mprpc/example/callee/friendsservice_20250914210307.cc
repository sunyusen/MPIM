#include <iostream>
#include <string>
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "friend.pb.h"
#include <vector>

class FriendService : public fixbug::FriendServiceRpc
{
public:
	std::vector<std::string> GetFriendsList(uint32_t userid)
	{
		std::cout << "do GetFriendsList service! userid:" << userid << std::endl;
		std::vector<std::string> friends;
		friends.push_back("zhangsan");
		friends.push_back("lisi");
		friends.push_back("wangwu");
		return friends;
	}

	void GetFriendsList(::google::protobuf::RpcController* controller,
                       const ::fixbug::GetFriendsListRequest* request,
                       ::fixbug::GetFriendsListResponse* response,
                       ::google::protobuf::Closure* done)
	{
		uint32_t userid = request->userid();
		std::vector<std::string> friends = GetFriendsList(userid);
		response->mutable_result()->set_errcode(0); // 设置错误码
		response->mutable_result()->set_errmsg(""); // 设置错误信息
		for (std::string &name: friends){
			std::string *p = response->add_friends();
			*p = name; // 添加好友名称到响应中
		}
		done->Run();
	}
};

int main(int argc, char **argv)
{
	LOG_INFO("first log message!");
	LOG_ERROR("%s:%s:%d", __FILE__, __FUNCTION__, __LINE__);
	// 调用框架的初始化操作
	MprpcApplication::Init(argc, argv);

	// provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
	RpcProvider provider;	//创建rpc服务发布者对象
	provider.NotifyService(new FriendService());	//注册FriendService服务到rpc框架中
	
	// 启动rpc服务发布节点Run以后 进程进入阻塞状态 等待远程的rpc调用请求
	provider.Run();

	return 0;
}