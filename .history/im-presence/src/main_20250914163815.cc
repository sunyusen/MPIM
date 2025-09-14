#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "presence_service.h"
#include <memory>

int main(int argc, char **argv)
{
	MprpcApplication::Init(argc, argv);
	RpcProvider p;
	p.NotifyService(std::make_unique<PresenceServiceImpl>());
	p.Run();	
	return 0;
}
