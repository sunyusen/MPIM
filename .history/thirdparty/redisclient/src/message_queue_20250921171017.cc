#include "message_queue.h"
#include "logger/logger.h"
#include <cstring>

namespace mpim {
namespace redis {

	
MessageQueue::MessageQueue() 
    : redis_client_(&RedisClient::GetInstance())
    , publish_context_(nullptr)
    , subscribe_context_(nullptr)
    , running_(false)
    , connected_(false) {
}

MessageQueue::~MessageQueue() {
    Stop();
    Disconnect();
}

bool MessageQueue::Connect() {
    if (connected_.load()) {
        return true;
    }
    
    // 创建发布连接
    publish_context_ = redisConnect("127.0.0.1", 6379);
    if (publish_context_ == nullptr || publish_context_->err) {
        if (publish_context_) {
            LOG_ERROR << "MessageQueue publish connection error: " << publish_context_->errstr;
            redisFree(publish_context_);
            publish_context_ = nullptr;
        }
        return false;
    }
    
    // 创建订阅连接
    subscribe_context_ = redisConnect("127.0.0.1", 6379);
    if (subscribe_context_ == nullptr || subscribe_context_->err) {
        if (subscribe_context_) {
            LOG_ERROR << "MessageQueue subscribe connection error: " << subscribe_context_->errstr;
            redisFree(subscribe_context_);
            subscribe_context_ = nullptr;
        }
        redisFree(publish_context_);
        publish_context_ = nullptr;
        return false;
    }
    
    connected_.store(true);
    LOG_INFO << "MessageQueue connected successfully";
    return true;
}

void MessageQueue::Disconnect() {
    Stop();
    
    if (publish_context_) {
        redisFree(publish_context_);
        publish_context_ = nullptr;
    }
    
    if (subscribe_context_) {
        redisFree(subscribe_context_);
        subscribe_context_ = nullptr;
    }
    
    connected_.store(false);
}

bool MessageQueue::IsConnected() const {
    return connected_.load() && publish_context_ && subscribe_context_;
}

bool MessageQueue::Publish(int channel, const std::string& message) {
    if (!IsConnected()) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(publish_context_, "PUBLISH %d %s", channel, message.c_str());
    if (reply == nullptr) {
        return false;
    }
    
    bool result = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return result;
}

bool MessageQueue::Subscribe(int channel) {
    if (!IsConnected()) {
        return false;
    }
    
    // 发送订阅命令
    if (redisAppendCommand(subscribe_context_, "SUBSCRIBE %d", channel) != REDIS_OK) {
        return false;
    }
    
    // 发送缓冲区数据
    int done = 0;
    while (!done) {
        if (redisBufferWrite(subscribe_context_, &done) != REDIS_OK) {
            return false;
        }
    }
    
    return true;
}

bool MessageQueue::Unsubscribe(int channel) {
    if (!IsConnected()) {
        return false;
    }
    
    // 发送取消订阅命令
    if (redisAppendCommand(subscribe_context_, "UNSUBSCRIBE %d", channel) != REDIS_OK) {
        return false;
    }
    
    // 发送缓冲区数据
    int done = 0;
    while (!done) {
        if (redisBufferWrite(subscribe_context_, &done) != REDIS_OK) {
            return false;
        }
    }
    
    return true;
}

void MessageQueue::SetNotifyHandler(std::function<void(int, std::string)> handler) {
    notify_handler_ = handler;
}

void MessageQueue::Start() {
    if (running_.load() || !IsConnected()) {
        return;
    }
    
    running_.store(true);
    observer_thread_ = std::thread(&MessageQueue::ObserverChannelMessage, this);
}

void MessageQueue::Stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    if (observer_thread_.joinable()) {
        observer_thread_.join();
    }
}

void MessageQueue::ObserverChannelMessage() {
    redisReply* reply = nullptr;
    
    while (running_.load()) {
        if (redisGetReply(subscribe_context_, (void**)&reply) != REDIS_OK) {
            break;
        }
        
        if (reply == nullptr) {
            continue;
        }
        
        if (reply->type == REDIS_REPLY_ARRAY && reply->elements >= 3) {
            if (strcmp(reply->element[0]->str, "message") == 0) {
                int channel = atoi(reply->element[1]->str);
                std::string message(reply->element[2]->str, reply->element[2]->len);
                
                if (notify_handler_) {
                    notify_handler_(channel, message);
                }
            }
        }
        
        freeReplyObject(reply);
    }
}

} // namespace redis
} // namespace mpim
