#pragma once
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Logging.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <cstddef>

// mprpc
#include "mprpcchannel.h"
#include "mprpccontroller.h"

// 业务 proto
#include "user.pb.h"
#include "presence.pb.h"
#include "message.pb.h"
#include "group.pb.h"

#include "redis.hpp"

/*
GatewayServer类是一个基于Muduo网络库实现的网关服务器，主要用于处理客户端连接、消息转发以及与后端服务的交互
*/
class GatewayServer {
public:
    // 统一成 “网关ID” 语义
    GatewayServer(muduo::net::EventLoop* loop,
                  const muduo::net::InetAddress& addr,
                  const std::string& nameArg);

	// 启动网关服务器，开始监听客户端连接
    void start();

private:
	using TcpConnectionPtr = muduo::net::TcpConnectionPtr;
	using WeakConn = std::weak_ptr<muduo::net::TcpConnection>;

    struct Session {
        bool authed{false};
        int64_t uid{0};
        std::string token;
    };

    // ---- muduo 回调 ----
	// 处理客户端连接的建立和断开
	// 1）当有新连接时，初始化会话。2）当连接断开时，清理相关资源
    void onConnection(const TcpConnectionPtr& conn);
	// 处理客户端发送的消息
	// 从buf中读取数据， 按行解析协议内容，并调用handleLine处理
    void onMessage(const TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp time);

    // ---- 文本协议处理 ----
	// 处理单行协议命令
    void handleLine(const TcpConnectionPtr& conn, const std::string& line);
	// 处理客户端登录请求
    bool handleLOGIN(const TcpConnectionPtr& conn, const std::vector<std::string>& toks);
	// 处理客户端发送消息请求
    bool handleSEND (const TcpConnectionPtr& conn, const std::vector<std::string>& toks);
	// 处理客户端拉取离线消息请求
    bool handlePULL (const TcpConnectionPtr& conn, const std::vector<std::string>& toks);
	// 处理客户端注册请求
	bool handleREGISTER(const TcpConnectionPtr &conn, const std::vector<std::string> &toks);
	// 处理客户端登出请求
	bool handleLOGOUT(const TcpConnectionPtr &conn, const std::vector<std::string> &toks);
	// 处理添加好友请求
	bool handleADDFRIEND(const TcpConnectionPtr &conn, const std::vector<std::string> &toks);
	// 处理获取好友列表请求
	bool handleGETFRIENDS(const TcpConnectionPtr &conn, const std::vector<std::string> &toks);
	// 处理创建群组请求
	bool handleCREATEGROUP(const TcpConnectionPtr &conn, const std::vector<std::string> &toks);
	// 处理加入群组请求
	bool handleJOINGROUP(const TcpConnectionPtr &conn, const std::vector<std::string> &toks);
	// 处理群组消息发送请求
	bool handleSENDGROUP(const TcpConnectionPtr &conn, const std::vector<std::string> &toks);


    // ---- 工具 ----
    static std::vector<std::string> split(const std::string& s, char delim);
    static std::vector<std::string> splitOnce(const std::string& s, char delim, size_t max_parts);
	void sendLine(const TcpConnectionPtr& conn, const std::string& line);
    Session& sessionOf(const TcpConnectionPtr& conn);
    static bool ok(const mpim::Result& r) { return r.code() == mpim::Code::Ok; }

	// 统一的网关通道号，保证和 presence使用的一致
	int gatewayChannel() const;

private:
    muduo::net::TcpServer server_;
    std::string gateway_id_;

    // 连接名 -> 会话
    std::unordered_map<std::string, Session> sessions_;
	// uid -> 连接（在线快速下发）
	std::unordered_map<int64_t, WeakConn> uid2conn_;

    // RPC stubs（各自使用一个 channel）
    std::unique_ptr<MprpcChannel> ch_user_;
    std::unique_ptr<MprpcChannel> ch_presence_;
    std::unique_ptr<MprpcChannel> ch_message_;

    std::unique_ptr<mpim::UserService_Stub>     user_;
    std::unique_ptr<mpim::PresenceService_Stub> presence_;
    std::unique_ptr<mpim::MessageService_Stub>  message_;

	// Redis： 订阅本网关通道，接收 Presence.Deliver 投递的在线消息
	Redis redis_;
};
