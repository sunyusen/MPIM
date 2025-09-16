#include "cache/business_cache.h"
#include "logger/logger.h"
#include <sstream>

namespace mpim {
namespace cache {

BusinessCacheImpl::BusinessCacheImpl(const std::string& key_prefix, const CacheConfig& config)
    : key_prefix_(key_prefix)
    , config_(config)
    , status_(CacheStatus::DISABLED)
    , cache_hits_(0)
    , cache_misses_(0)
    , degraded_requests_(0)
    , last_health_check_(std::chrono::steady_clock::now())
{
}

bool BusinessCacheImpl::Connect(const std::string& ip, int port) {
    if (status_ != CacheStatus::DISABLED) {
        return true;
    }
    
    if (cache_manager_.Connect(ip, port)) {
        status_ = CacheStatus::CONNECTED;
        LOG_INFO << "BusinessCache connected successfully with prefix: " << key_prefix_;
        return true;
    }
    
    if (config_.enable_degraded_mode) {
        status_ = CacheStatus::DEGRADED;
        LOG_WARN << "BusinessCache connection failed, switching to degraded mode for prefix: " << key_prefix_;
        return true;
    }
    
    LOG_ERROR << "BusinessCache connection failed and degraded mode disabled for prefix: " << key_prefix_;
    return false;
}

void BusinessCacheImpl::Disconnect() {
    cache_manager_.Disconnect();
    status_ = CacheStatus::DISABLED;
    LOG_INFO << "BusinessCache disconnected for prefix: " << key_prefix_;
}

bool BusinessCacheImpl::IsConnected() const {
    return status_ != CacheStatus::DISABLED;
}

bool BusinessCacheImpl::Set(const std::string& key, const std::string& value, int ttl) {
    if (!config_.enable_cache) {
        return false;
    }
    
    // 健康检查
    HealthCheck();
    
    // 尝试写入缓存
    if (status_ == CacheStatus::CONNECTED) {
        std::string full_key = MakeKey(key);
        int actual_ttl = (ttl > 0) ? ttl : config_.default_ttl;
        
        if (cache_manager_.Setex(full_key, actual_ttl, value)) {
            return true;
        }
        
        // 缓存写入失败，切换到降级模式
        if (config_.enable_degraded_mode) {
            status_ = CacheStatus::DEGRADED;
            LOG_WARN << "Cache write failed, switching to degraded mode for prefix: " << key_prefix_;
        }
    }
    
    // 降级模式：不写入缓存，直接返回成功
    if (status_ == CacheStatus::DEGRADED) {
        return true;
    }
    
    return false;
}

std::string BusinessCacheImpl::Get(const std::string& key) {
    if (!config_.enable_cache) {
        return "";
    }
    
    // 健康检查
    HealthCheck();
    
    // 尝试从缓存获取
    if (status_ == CacheStatus::CONNECTED) {
        std::string full_key = MakeKey(key);
        std::string data = cache_manager_.Get(full_key);
        
        if (!data.empty()) {
            cache_hits_.fetch_add(1);
            return data;
        }
        
        cache_misses_.fetch_add(1);
    }
    
    // 降级模式：尝试从数据库获取
    if (status_ == CacheStatus::DEGRADED && degraded_callback_) {
        degraded_requests_.fetch_add(1);
        return degraded_callback_(key);
    }
    
    return "";
}

bool BusinessCacheImpl::Del(const std::string& key) {
    if (!config_.enable_cache || status_ == CacheStatus::DISABLED) {
        return false;
    }
    
    std::string full_key = MakeKey(key);
    return cache_manager_.Del(full_key);
}

CacheStatus BusinessCacheImpl::GetStatus() const {
    return status_.load();
}

void BusinessCacheImpl::SetStatus(CacheStatus status) {
    status_.store(status);
}

void BusinessCacheImpl::HealthCheck() {
    auto now = std::chrono::steady_clock::now();
    if (now - last_health_check_ < std::chrono::milliseconds(config_.health_check_interval_ms)) {
        return;
    }
    
    if (status_ == CacheStatus::CONNECTED) {
        // 检查Redis连接
        if (!cache_manager_.IsConnected()) {
            if (config_.enable_degraded_mode) {
                status_ = CacheStatus::DEGRADED;
                LOG_WARN << "Redis connection lost, switching to degraded mode for prefix: " << key_prefix_;
            } else {
                status_ = CacheStatus::DISABLED;
                LOG_ERROR << "Redis connection lost and degraded mode disabled for prefix: " << key_prefix_;
            }
        }
    } else if (status_ == CacheStatus::DEGRADED) {
        // 尝试恢复连接
        if (cache_manager_.IsConnected()) {
            status_ = CacheStatus::CONNECTED;
            LOG_INFO << "Redis connection recovered, switching back to cache mode for prefix: " << key_prefix_;
        }
    }
    
    last_health_check_ = now;
    LogMetrics();
}

int BusinessCacheImpl::GetCacheHits() const {
    return cache_hits_.load();
}

int BusinessCacheImpl::GetCacheMisses() const {
    return cache_misses_.load();
}

int BusinessCacheImpl::GetDegradedRequests() const {
    return degraded_requests_.load();
}

void BusinessCacheImpl::SetDegradedCallback(std::function<std::string(const std::string&)> callback) {
    degraded_callback_ = callback;
}

std::string BusinessCacheImpl::MakeKey(const std::string& key) {
    return key_prefix_ + ":" + key;
}

void BusinessCacheImpl::LogMetrics() {
    int hits = cache_hits_.load();
    int misses = cache_misses_.load();
    int degraded = degraded_requests_.load();
    
    if (hits + misses > 0) {
        double hit_rate = static_cast<double>(hits) / (hits + misses) * 100.0;
        LOG_INFO << "Cache metrics for " << key_prefix_ 
                 << " - Hits: " << hits 
                 << ", Misses: " << misses 
                 << ", Hit Rate: " << hit_rate << "%"
                 << ", Degraded: " << degraded;
    }
    
    // 如果降级请求过多，发送告警
    if (degraded > 100) {
        LOG_ERROR << "High number of degraded requests for " << key_prefix_ << ": " << degraded;
    }
}

} // namespace cache
} // namespace mpim
