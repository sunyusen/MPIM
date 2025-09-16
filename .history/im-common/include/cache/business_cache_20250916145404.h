#pragma once
#include "generic_cache.h"
#include <string>
#include <memory>
#include <functional>

namespace mpim {
namespace cache {

// 业务缓存接口（不依赖具体protobuf类型）
class BusinessCache {
public:
    virtual ~BusinessCache() = default;
    
    // 连接管理
    virtual bool Connect(const std::string& ip = "127.0.0.1", int port = 6379) = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;
    
    // 缓存操作
    virtual bool Set(const std::string& key, const std::string& value, int ttl = 3600) = 0;
    virtual std::string Get(const std::string& key) = 0;
    virtual bool Del(const std::string& key) = 0;
    
    // 状态管理
    virtual CacheStatus GetStatus() const = 0;
    virtual void SetStatus(CacheStatus status) = 0;
    
    // 健康检查
    virtual void HealthCheck() = 0;
    
    // 统计信息
    virtual int GetCacheHits() const = 0;
    virtual int GetCacheMisses() const = 0;
    virtual int GetDegradedRequests() const = 0;
    
    // 设置降级回调
    virtual void SetDegradedCallback(std::function<std::string(const std::string&)> callback) = 0;
};

// 业务缓存实现
class BusinessCacheImpl : public BusinessCache {
public:
    BusinessCacheImpl(const std::string& key_prefix, const CacheConfig& config = CacheConfig{});
    virtual ~BusinessCacheImpl() = default;
    
    // 实现BusinessCache接口
    bool Connect(const std::string& ip = "127.0.0.1", int port = 6379) override;
    void Disconnect() override;
    bool IsConnected() const override;
    
    bool Set(const std::string& key, const std::string& value, int ttl = 3600) override;
    std::string Get(const std::string& key) override;
    bool Del(const std::string& key) override;
    
    CacheStatus GetStatus() const override;
    void SetStatus(CacheStatus status) override;
    
    void HealthCheck() override;
    
    int GetCacheHits() const override;
    int GetCacheMisses() const override;
    int GetDegradedRequests() const override;
    
    void SetDegradedCallback(std::function<std::string(const std::string&)> callback) override;

private:
    std::string key_prefix_;
    CacheConfig config_;
    mpim::redis::CacheManager cache_manager_;
    std::atomic<CacheStatus> status_;
    std::atomic<int> cache_hits_;
    std::atomic<int> cache_misses_;
    std::atomic<int> degraded_requests_;
    std::chrono::steady_clock::time_point last_health_check_;
    std::function<std::string(const std::string&)> degraded_callback_;
    
    std::string MakeKey(const std::string& key);
    void LogMetrics();
};

} // namespace cache
} // namespace mpim
