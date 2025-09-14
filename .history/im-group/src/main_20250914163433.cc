#include "group_service.h"
#include "rpcprovider.h"
#include "mprpcapplication.h"
#include <iostream>
#include <memory>

int main(int argc, char **argv)
{
    // 初始化框架
    MprpcApplication::Init(argc, argv);

    // 创建服务提供者
    RpcProvider provider;
    
    // 发布服务
    // GroupServiceImpl groupService;
    // provider.NotifyService(&groupService);
	provider.NotifyService(new GroupServiceImpl());

    // 启动服务
    provider.Run();

    return 0;
}
