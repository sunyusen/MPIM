#pragma once
#include "cache_manager.h"
#include <string>
#include <memory>
#include <functional>

namespace mpim {
namespace presence {

// 路由缓存类 - 专门处理用户路由相关的缓存
class RouteCache {
public:
    RouteCache();
    ~RouteCache();
    
    // 连接管理
    bool Connect(const std::string& ip = "127.0.0.1", int port = 6379);
    void Disconnect();
    bool IsConnected() const;
    
    // 路由信息缓存
    std::string GetRoute(int64_t user_id);
    bool SetRoute(int64_t user_id, const std::string& gateway_id, int ttl = 120);
    bool DelRoute(int64_t user_id);
    
    // 设置降级回调
    void SetDegradedCallback(std::function<std::string(const std::string&)> callback);
    
    // 统计信息
    void LogMetrics();

private:
    mpim::redis::CacheManager cache_manager_;
    bool connected_;
    std::function<std::string(const std::string&)> degraded_callback_;
    
    // 缓存键生成
    std::string RouteKey(int64_t user_id);
};

} // namespace presence
} // namespace mpim
