#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "user_service.h"
#include "health_service.h"
int main(int argc, char **argv)
{
	MprpcApplication::Init(argc, argv);
	RpcProvider p;
	p.NotifyService(new UserServiceImpl());		// 把 UserServiceImpl 服务发布到 rpc 节点上
	p.NotifyService(new HealthServiceImpl());	// 把 HealthServiceImpl 服务发布到 rpc 节点上
	p.Run();

	return 0;
}
