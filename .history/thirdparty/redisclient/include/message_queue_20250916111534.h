#pragma once
#include "redis_client.h"
#include <functional>
#include <thread>
#include <atomic>

namespace mpim {
namespace redis {

// 消息队列功能
class MessageQueue {
public:
    MessageQueue();
    ~MessageQueue();
    
    bool Connect();
    void Disconnect();
    bool IsConnected() const;
    
    // 发布消息
    bool Publish(int channel, const std::string& message);
    
    // 订阅频道
    bool Subscribe(int channel);
    bool Unsubscribe(int channel);
    
    // 设置消息处理回调
    void SetNotifyHandler(std::function<void(int, std::string)> handler);
    
    // 启动/停止消息监听
    void Start();
    void Stop();
    
private:
    void ObserverChannelMessage();
    
    RedisClient* redis_client_;
    redisContext* publish_context_;
    redisContext* subscribe_context_;
    std::function<void(int, std::string)> notify_handler_;
    std::thread observer_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;
};

} // namespace redis
} // namespace mpim
