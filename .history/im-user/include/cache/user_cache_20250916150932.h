#pragma once
#include "cache_manager.h"
#include <string>
#include <memory>
#include <functional>

namespace mpim {
namespace user {

// 用户缓存类 - 专门处理用户相关的缓存
class UserCache {
public:
    UserCache();
    ~UserCache();
    
    // 连接管理
    bool Connect(const std::string& ip = "127.0.0.1", int port = 6379);
    void Disconnect();
    bool IsConnected() const;
    
    // 用户信息缓存
    std::string GetUserInfo(const std::string& username);
    bool SetUserInfo(const std::string& username, const std::string& user_data, int ttl = 3600);
    bool DelUserInfo(const std::string& username);
    
    // 好友关系缓存
    std::string GetFriends(int64_t user_id);
    bool SetFriends(int64_t user_id, const std::string& friends_data, int ttl = 1800);
    bool DelFriends(int64_t user_id);
    
    // 设置降级回调
    void SetDegradedCallback(std::function<std::string(const std::string&)> callback);
    
    // 统计信息
    void LogMetrics();

private:
    mpim::redis::CacheManager cache_manager_;
    bool connected_;
    std::function<std::string(const std::string&)> degraded_callback_;
    
    // 缓存键生成
    std::string UserInfoKey(const std::string& username);
    std::string FriendsKey(int64_t user_id);
};

} // namespace user
} // namespace mpim
