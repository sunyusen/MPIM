#include "redis_client.h"
#include <cstdarg>
#include "logger/logger.h"

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
            LOG_ERROR << "Redis connection error: " << context_->errstr;
            redisFree(context_);
            context_ = nullptr;
        } else {
            LOG_ERROR << "Redis connection error: can't allocate redis context";
        }
        return false;
    }
    
    LOG_INFO << "Redis connected successfully to " << host << ":" << port;
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
    std::lock_guard<std::mutex> lock(mutex_);
    if (!context_) {
        return false;
    }
    
    va_list args;
    va_start(args, format);
    redisReply* reply = (redisReply*)redisvCommand(context_, format, args);
    va_end(args);
    
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
