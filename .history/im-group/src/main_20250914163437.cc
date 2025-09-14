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
    
    // 发布服务 - 使用智能指针管理内存
    auto groupService = std::make_unique<GroupServiceImpl>();
    provider.NotifyService(groupService.get());

    // 启动服务
    provider.Run();

    return 0;
}
