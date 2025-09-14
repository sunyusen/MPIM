#include "group_service.h"
#include "health_service.h"
#include "rpcprovider.h"
#include "rpcapplication.h"
#include <iostream>

int main(int argc, char **argv)
{
    // 初始化框架
    RpcApplication::Init(argc, argv);

    // 创建服务提供者
    RpcProvider provider;
    
    // 发布服务
    GroupServiceImpl groupService;
    provider.NotifyService(&groupService);
    
    HealthServiceImpl healthService;
    provider.NotifyService(&healthService);

    // 启动服务
    provider.Run();

    return 0;
}
