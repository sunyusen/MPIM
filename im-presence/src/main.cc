#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "presence_service.h"
int main(int argc, char **argv)
{
	MprpcApplication::Init(argc, argv);
	RpcProvider p;
	p.NotifyService(new PresenceServiceImpl());
	p.Run();
	return 0;
}
