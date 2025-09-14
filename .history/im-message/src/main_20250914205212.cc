#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "message_service.h"   // 你的 MessageServiceImpl 声明
#include "logger/log_init.h"
#include <memory>

int main(int argc, char** argv) {
    // 读取 mprpc 配置（如 zookeeper 地址、日志等）
    MprpcApplication::Init(argc, argv);

    // 注册并发布消息服务
    RpcProvider provider;
    provider.NotifyService(std::make_unique<MessageServiceImpl>());  // 派生自 mpim::MessageService
    provider.Run();  // 阻塞运行
    return 0;
}
