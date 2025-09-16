#pragma once
#include "cache_manager.h"
#include <string>
#include <memory>
#include <functional>

namespace mpim {
namespace group {

// 群组缓存类 - 专门处理群组相关的缓存
class GroupCache {
public:
    GroupCache();
    ~GroupCache();
    
    // 连接管理
    bool Connect(const std::string& ip = "127.0.0.1", int port = 6379);
    void Disconnect();
    bool IsConnected() const;
    
    // 群组信息缓存
    std::string GetGroupInfo(int64_t group_id);
    bool SetGroupInfo(int64_t group_id, const std::string& group_data, int ttl = 3600);
    bool DelGroupInfo(int64_t group_id);
    
    // 群组成员缓存
    std::string GetGroupMembers(int64_t group_id);
    bool SetGroupMembers(int64_t group_id, const std::string& members_data, int ttl = 1800);
    bool DelGroupMembers(int64_t group_id);
    
    // 用户群组列表缓存
    std::string GetUserGroups(int64_t user_id);
    bool SetUserGroups(int64_t user_id, const std::string& groups_data, int ttl = 1800);
    bool DelUserGroups(int64_t user_id);
    
    // 设置降级回调
    void SetDegradedCallback(std::function<std::string(const std::string&)> callback);
    
    // 统计信息
    void LogMetrics();

private:
    mpim::redis::CacheManager cache_manager_;
    bool connected_;
    std::function<std::string(const std::string&)> degraded_callback_;
    
    // 缓存键生成
    std::string GroupInfoKey(int64_t group_id);
    std::string GroupMembersKey(int64_t group_id);
    std::string UserGroupsKey(int64_t user_id);
};

} // namespace group
} // namespace mpim
