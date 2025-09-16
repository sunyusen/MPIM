#pragma once
#include <hiredis/hiredis.h>
#include <string>
#include <mutex>
#include <memory>

namespace mpim {
namespace redis {

// 底层Redis连接管理
class RedisClient {
public:
    static RedisClient& GetInstance();
    
    bool Connect(const std::string& host = "127.0.0.1", int port = 6379);
    void Disconnect();
    bool IsConnected() const;
    
    // 基础Redis操作
    redisReply* ExecuteCommand(const char* format, ...);
    bool ExecuteCommandBool(const char* format, ...);
    
    // 获取连接上下文（用于需要保持连接的操作）
    redisContext* GetContext();
    
private:
    RedisClient() = default;
    ~RedisClient();
    RedisClient(const RedisClient&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;
    
    redisContext* context_;
    std::string host_;
    int port_;
    mutable std::mutex mutex_;
};

} // namespace redis
} // namespace mpim
