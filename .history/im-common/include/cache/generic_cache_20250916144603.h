#pragma once
#include "cache_manager.h"
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>

namespace mpim {
namespace cache {

// 缓存状态枚举
enum class CacheStatus {
    CONNECTED,      // Redis正常
    DEGRADED,       // Redis故障，降级到数据库
    DISABLED        // 缓存完全禁用
};

// 缓存配置
struct CacheConfig {
    bool enable_cache = true;           // 是否启用缓存
    bool enable_degraded_mode = true;   // 是否启用降级模式
    int cache_timeout_ms = 1000;        // 缓存操作超时时间
    int health_check_interval_ms = 60000; // 健康检查间隔
    int max_retry_attempts = 3;         // 最大重试次数
    int default_ttl = 3600;             // 默认TTL（秒）
};

// 通用缓存模板类
template<typename T>
class GenericCache {
public:
    GenericCache(const std::string& key_prefix, const CacheConfig& config = CacheConfig{});
    ~GenericCache();

    // 连接管理
    bool Connect(const std::string& ip = "127.0.0.1", int port = 6379);
    void Disconnect();
    bool IsConnected() const;
    
    // 缓存操作
    std::shared_ptr<T> Get(const std::string& key);
    bool Set(const std::string& key, const T& value, int ttl = -1);
    bool Del(const std::string& key);
    
    // 状态管理
    CacheStatus GetStatus() const { return status_; }
    void SetStatus(CacheStatus status) { status_ = status; }
    
    // 健康检查
    void HealthCheck();
    
    // 统计信息
    int GetCacheHits() const { return cache_hits_.load(); }
    int GetCacheMisses() const { return cache_misses_.load(); }
    int GetDegradedRequests() const { return degraded_requests_.load(); }
    
    // 设置降级回调
    void SetDegradedCallback(std::function<std::shared_ptr<T>(const std::string&)> callback);
    
    // 序列化/反序列化接口
    virtual std::string Serialize(const T& value) = 0;
    virtual std::shared_ptr<T> Deserialize(const std::string& data) = 0;

private:
    std::string key_prefix_;
    CacheConfig config_;
    mpim::redis::CacheManager cache_manager_;
    std::atomic<CacheStatus> status_;
    std::atomic<int> cache_hits_;
    std::atomic<int> cache_misses_;
    std::atomic<int> degraded_requests_;
    std::chrono::steady_clock::time_point last_health_check_;
    std::function<std::shared_ptr<T>(const std::string&)> degraded_callback_;
    
    std::string MakeKey(const std::string& key);
    void LogMetrics();
};

} // namespace cache
} // namespace mpim
