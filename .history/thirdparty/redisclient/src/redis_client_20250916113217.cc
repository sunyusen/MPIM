#include "redis_client.h"
#include <cstdarg>
#include <iostream>

namespace mpim {
namespace redis {

RedisClient& RedisClient::GetInstance() {
    static RedisClient instance;
    return instance;
}

RedisClient::~RedisClient() {
    Disconnect();
}

bool RedisClient::Connect(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (context_ != nullptr) {
        redisFree(context_);
    }
    
    host_ = host;
    port_ = port;
    context_ = redisConnect(host.c_str(), port);
    
    if (context_ == nullptr || context_->err) {
        if (context_) {
            std::cerr << "Redis connection error: " << context_->errstr << std::endl;
            redisFree(context_);
            context_ = nullptr;
        } else {
            std::cerr << "Redis connection error: can't allocate redis context" << std::endl;
        }
        return false;
    }
    
    std::cout << "Redis connected successfully to " << host << ":" << port << std::endl;
    return true;
}

void RedisClient::Disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (context_ != nullptr) {
        redisFree(context_);
        context_ = nullptr;
    }
}

bool RedisClient::IsConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return context_ != nullptr && !context_->err;
}

redisReply* RedisClient::ExecuteCommand(const char* format, ...) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!context_ || context_->err) {
        return nullptr;
    }
    
    va_list args;
    va_start(args, format);
    redisReply* reply = (redisReply*)redisvCommand(context_, format, args);
    va_end(args);
    
    return reply;
}

bool RedisClient::ExecuteCommandBool(const char* format, ...) {
    redisReply* reply = ExecuteCommand(format, ...);
    if (reply == nullptr) {
        return false;
    }
    
    bool result = (reply->type != REDIS_REPLY_ERROR);
    freeReplyObject(reply);
    return result;
}

redisContext* RedisClient::GetContext() {
    std::lock_guard<std::mutex> lock(mutex_);
    return context_;
}

} // namespace redis
} // namespace mpim
