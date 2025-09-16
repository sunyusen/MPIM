#include "user_cache.h"
#include "logger/logger.h"

namespace mpim {
namespace user {

UserCache::UserCache() : connected_(false) {
}

UserCache::~UserCache() {
    Disconnect();
}

bool UserCache::Connect(const std::string& ip, int port) {
    if (connected_) {
        return true;
    }
    
    if (cache_manager_.Connect(ip, port)) {
        connected_ = true;
        LOG_INFO << "UserCache connected successfully";
        return true;
    }
    
    LOG_WARN << "UserCache connection failed, will use degraded mode";
    return false;
}

void UserCache::Disconnect() {
    if (connected_) {
        cache_manager_.Disconnect();
        connected_ = false;
        LOG_INFO << "UserCache disconnected";
    }
}

bool UserCache::IsConnected() const {
    return connected_;
}

std::string UserCache::GetUserInfo(const std::string& username) {
    if (!connected_) {
        // 降级模式
        if (degraded_callback_) {
            return degraded_callback_(UserInfoKey(username));
        }
        return "";
    }
    
    std::string key = UserInfoKey(username);
    return cache_manager_.Get(key);
}

bool UserCache::SetUserInfo(const std::string& username, const std::string& user_data, int ttl) {
    if (!connected_) {
        return false;
    }
    
    std::string key = UserInfoKey(username);
    return cache_manager_.Setex(key, ttl, user_data);
}

bool UserCache::DelUserInfo(const std::string& username) {
    if (!connected_) {
        return false;
    }
    
    std::string key = UserInfoKey(username);
    return cache_manager_.Del(key);
}

std::string UserCache::GetFriends(int64_t user_id) {
    if (!connected_) {
        // 降级模式
        if (degraded_callback_) {
            return degraded_callback_(FriendsKey(user_id));
        }
        return "";
    }
    
    std::string key = FriendsKey(user_id);
    return cache_manager_.Get(key);
}

bool UserCache::SetFriends(int64_t user_id, const std::string& friends_data, int ttl) {
    if (!connected_) {
        return false;
    }
    
    std::string key = FriendsKey(user_id);
    return cache_manager_.Setex(key, ttl, friends_data);
}

bool UserCache::DelFriends(int64_t user_id) {
    if (!connected_) {
        return false;
    }
    
    std::string key = FriendsKey(user_id);
    return cache_manager_.Del(key);
}

void UserCache::SetDegradedCallback(std::function<std::string(const std::string&)> callback) {
    degraded_callback_ = callback;
}

void UserCache::LogMetrics() {
    LOG_INFO << "UserCache metrics logged";
}

std::string UserCache::UserInfoKey(const std::string& username) {
    return "user:" + username;
}

std::string UserCache::FriendsKey(int64_t user_id) {
    return "friends:" + std::to_string(user_id);
}

} // namespace user
} // namespace mpim
