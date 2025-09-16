#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "../include/user_service.h"
#include "logger/log_init.h"
#include <memory>

int main(int argc, char **argv)
{
	mpim::logger::LogInit::InitDefault("im-userd");
	MprpcApplication::Init(argc, argv);
	
	// 创建服务实例
	auto userService = std::make_unique<UserServiceImpl>();
	
	// 服务启动时将所有用户状态设为离线（防止上次异常退出导致的状态不一致）
	userService->resetAllUsersToOffline();
	
	RpcProvider p;
	p.NotifyService(std::move(userService));		// 把 UserServiceImpl 服务发布到 rpc 节点上
	p.Run();

	return 0;
}
