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

// 好友关系缓存实现
std::string UserCache::GetFriends(int64_t user_id) {
    if (!IsConnected()) return "";
    return cache_manager_.Get(FriendsKey(user_id));
}

bool UserCache::SetFriends(int64_t user_id, const std::string& friends_data, int ttl) {
    if (!IsConnected()) return false;
    return cache_manager_.Setex(FriendsKey(user_id), ttl, friends_data);
}

bool UserCache::DelFriends(int64_t user_id) {
    if (!IsConnected()) return false;
    return cache_manager_.Del(FriendsKey(user_id));
}

bool UserCache::AddFriend(int64_t user_id, int64_t friend_id) {
    if (!IsConnected()) return false;
    return cache_manager_.Sadd(FriendsKey(user_id), std::to_string(friend_id));
}

bool UserCache::RemoveFriend(int64_t user_id, int64_t friend_id) {
    if (!IsConnected()) return false;
    return cache_manager_.Srem(FriendsKey(user_id), std::to_string(friend_id));
}

// 用户状态缓存实现
std::string UserCache::GetUserStatus(int64_t user_id) {
    if (!IsConnected()) return "";
    return cache_manager_.Get(UserStatusKey(user_id));
}

bool UserCache::SetUserStatus(int64_t user_id, const std::string& status, int ttl) {
    if (!IsConnected()) return false;
    return cache_manager_.Setex(UserStatusKey(user_id), ttl, status);
}

bool UserCache::DelUserStatus(int64_t user_id) {
    if (!IsConnected()) return false;
    return cache_manager_.Del(UserStatusKey(user_id));
}

// 用户名存在性缓存实现
bool UserCache::IsUsernameExists(const std::string& username) {
    if (!IsConnected()) return false;
    std::string result = cache_manager_.Get(UsernameExistsKey(username));
    return result == "1";
}

bool UserCache::SetUsernameExists(const std::string& username, bool exists, int ttl) {
    if (!IsConnected()) return false;
    return cache_manager_.Setex(UsernameExistsKey(username), ttl, exists ? "1" : "0");
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

std::string UserCache::UserStatusKey(int64_t user_id) {
    return "user_status:" + std::to_string(user_id);
}

std::string UserCache::UsernameExistsKey(const std::string& username) {
    return "username_exists:" + username;
}

} // namespace user
} // namespace mpim
