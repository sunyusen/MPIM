#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "presence_service.h"
#include "logger/log_init.h"
#include <memory>

int main(int argc, char **argv)
{
	// 初始化日志系统
	mpim::logger::LogInit::InitDefault("im-presenced");
	
	MprpcApplication::Init(argc, argv);
	RpcProvider p;
	p.NotifyService(std::make_unique<PresenceServiceImpl>());
	p.Run();
	return 0;
}
