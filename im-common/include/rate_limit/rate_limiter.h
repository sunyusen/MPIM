#pragma once
#include <string>
#include <map>
#include <chrono>
#include <atomic>

namespace mpim {
namespace rate_limit {

// 令牌桶限流器
class TokenBucket {
public:
    TokenBucket(int capacity, int refill_rate);
    
    // 尝试获取令牌
    bool TryConsume(int tokens = 1);
    
    // 获取当前令牌数
    int GetCurrentTokens() const;
    
    // 重置桶
    void Reset();

private:
    int capacity_;
    int refill_rate_;
    std::atomic<int> tokens_;
    std::chrono::system_clock::time_point last_refill_;
    mutable std::mutex mutex_;
};

// 滑动窗口限流器
class SlidingWindow {
public:
    SlidingWindow(int window_size_seconds, int max_requests);
    
    // 检查是否允许请求
    bool IsAllowed(const std::string& key);
    
    // 清理过期记录
    void Cleanup();

private:
    int window_size_seconds_;
    int max_requests_;
    std::map<std::string, std::vector<std::chrono::system_clock::time_point>> requests_;
    std::mutex mutex_;
};

// 限流管理器
class RateLimiter {
public:
    static RateLimiter& GetInstance();
    
    // 检查用户限流
    bool CheckUserLimit(const std::string& user_id);
    
    // 检查IP限流
    bool CheckIPLimit(const std::string& ip);
    
    // 检查接口限流
    bool CheckAPILimit(const std::string& api_path);

private:
    TokenBucket user_bucket_;
    TokenBucket ip_bucket_;
    SlidingWindow api_window_;
};

} // namespace rate_limit
} // namespace mpim
