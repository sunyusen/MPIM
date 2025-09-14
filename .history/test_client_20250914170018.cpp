#include <iostream>
#include <string>
#include "mprpc/src/include/mprpcapplication.h"
#include "mprpc/src/include/rpcprovider.h"
#include "mprpc/src/include/mprpcchannel.h"
#include "mprpc/src/include/mprpccontroller.h"
#include "im-presence/src/presence.pb.h"

int main() {
    // 初始化RPC框架
    MprpcApplication::Init(0, nullptr);
    
    // 创建RPC通道
    MprpcChannel channel;
    channel.Init("127.0.0.1", 8012);
    
    // 创建PresenceService客户端
    mpim::PresenceService_Stub stub(&channel);
    
    // 测试BindRoute
    mpim::BindRouteReq bind_req;
    mpim::BindRouteResp bind_resp;
    MprpcController bind_controller;
    
    bind_req.set_user_id(5);
    bind_req.set_gateway_id("gateway-1");
    
    std::cout << "Testing BindRoute for user 5..." << std::endl;
    stub.BindRoute(&bind_controller, &bind_req, &bind_resp, nullptr);
    
    if (bind_controller.Failed()) {
        std::cout << "BindRoute failed: " << bind_controller.ErrorText() << std::endl;
    } else {
        std::cout << "BindRoute result code: " << bind_resp.result().code() << std::endl;
        std::cout << "BindRoute result msg: " << bind_resp.result().msg() << std::endl;
    }
    
    // 测试QueryRoute
    mpim::QueryRouteReq query_req;
    mpim::QueryRouteResp query_resp;
    MprpcController query_controller;
    
    query_req.set_user_id(5);
    
    std::cout << "Testing QueryRoute for user 5..." << std::endl;
    stub.QueryRoute(&query_controller, &query_req, &query_resp, nullptr);
    
    if (query_controller.Failed()) {
        std::cout << "QueryRoute failed: " << query_controller.ErrorText() << std::endl;
    } else {
        std::cout << "QueryRoute result code: " << query_resp.result().code() << std::endl;
        std::cout << "QueryRoute result msg: " << query_resp.result().msg() << std::endl;
        std::cout << "QueryRoute gateway_id: " << query_resp.gateway_id() << std::endl;
    }
    
    return 0;
}
