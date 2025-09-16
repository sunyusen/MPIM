#include "cache/route_cache.h"
#include "logger/logger.h"

namespace mpim {
namespace presence {

RouteCache::RouteCache() : connected_(false) {
}

RouteCache::~RouteCache() {
    Disconnect();
}

bool RouteCache::Connect(const std::string& ip, int port) {
    if (connected_) {
        return true;
    }
    
    if (cache_manager_.Connect(ip, port)) {
        connected_ = true;
        LOG_INFO << "RouteCache connected successfully";
        return true;
    }
    
    LOG_WARN << "RouteCache connection failed, will use degraded mode";
    return false;
}

void RouteCache::Disconnect() {
    if (connected_) {
        cache_manager_.Disconnect();
        connected_ = false;
        LOG_INFO << "RouteCache disconnected";
    }
}

bool RouteCache::IsConnected() const {
    return connected_;
}

std::string RouteCache::GetRoute(int64_t user_id) {
    if (!connected_) {
        if (degraded_callback_) {
            return degraded_callback_(RouteKey(user_id));
        }
        return "";
    }
    
    std::string key = RouteKey(user_id);
    return cache_manager_.Get(key);
}

bool RouteCache::SetRoute(int64_t user_id, const std::string& gateway_id, int ttl) {
    if (!connected_) {
        return false;
    }
    
    std::string key = RouteKey(user_id);
    return cache_manager_.Setex(key, ttl, gateway_id);
}

bool RouteCache::DelRoute(int64_t user_id) {
    if (!connected_) {
        return false;
    }
    
    std::string key = RouteKey(user_id);
    return cache_manager_.Del(key);
}

void RouteCache::SetDegradedCallback(std::function<std::string(const std::string&)> callback) {
    degraded_callback_ = callback;
}

void RouteCache::LogMetrics() {
    LOG_INFO << "RouteCache metrics logged";
}

std::string RouteCache::RouteKey(int64_t user_id) {
    return "route:" + std::to_string(user_id);
}

} // namespace presence
} // namespace mpim
