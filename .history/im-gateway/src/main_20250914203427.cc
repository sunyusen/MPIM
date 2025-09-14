#include "mprpcapplication.h"
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include "gatewayServer.h"
#include "logger/log_init.h"

int main(int argc, char** argv) {
  MprpcApplication::Init(argc, argv);

  muduo::net::EventLoop loop;
  muduo::net::InetAddress addr(9000);             // 对外端口，可放到 conf
  GatewayServer server(&loop, addr, "gateway-1");  // 网关实例ID，可放到 conf
  server.start();
  loop.loop();
  return 0;
}
