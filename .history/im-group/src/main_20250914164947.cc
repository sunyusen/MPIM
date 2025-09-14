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
    
    // 发布服务 - 使用智能指针管理内存，RpcProvider会自动管理生命周期
    provider.NotifyService(std::make_unique<GroupServiceImpl>());

    // 启动服务nam1z'czc
    provider.Run();

    return 0;
}
