#pragma once
#include "cache_manager.h"
#include "group.pb.h"
#include <string>
#include <memory>
#include <vector>

namespace mpim {
namespace cache {

// 用户信息缓存服务
class UserCache {
public:
    UserCache();
    ~UserCache();
    
    bool Connect();
    void Disconnect();
    
    // 用户基本信息缓存（简单的字符串存储）
    std::string GetUserInfo(int64_t user_id);
    bool SetUserInfo(int64_t user_id, const std::string& user_info, int ttl = 3600);
    bool DelUserInfo(int64_t user_id);
    
    // 好友关系缓存
    std::vector<int64_t> GetFriends(int64_t user_id);
    bool SetFriends(int64_t user_id, const std::vector<int64_t>& friends, int ttl = 1800);
    bool AddFriend(int64_t user_id, int64_t friend_id);
    bool RemoveFriend(int64_t user_id, int64_t friend_id);
    
    // 群组信息缓存
    std::shared_ptr<mpim::GroupInfo> GetGroupInfo(int64_t group_id);
    bool SetGroupInfo(int64_t group_id, const mpim::GroupInfo& group_info, int ttl = 3600);
    bool DelGroupInfo(int64_t group_id);
    
    // 群组成员缓存
    std::vector<int64_t> GetGroupMembers(int64_t group_id);
    bool SetGroupMembers(int64_t group_id, const std::vector<int64_t>& members, int ttl = 1800);
    bool AddGroupMember(int64_t group_id, int64_t user_id);
    bool RemoveGroupMember(int64_t group_id, int64_t user_id);
    
private:
    mpim::redis::CacheManager cache_manager_;
    bool connected_;
    
    // 缓存键生成
    std::string UserInfoKey(int64_t user_id);
    std::string FriendsKey(int64_t user_id);
    std::string GroupInfoKey(int64_t group_id);
    std::string GroupMembersKey(int64_t group_id);
    
    // 序列化/反序列化
    std::string SerializeGroupInfo(const mpim::GroupInfo& group_info);
    std::shared_ptr<mpim::GroupInfo> DeserializeGroupInfo(const std::string& data);
    std::string SerializeInt64Vector(const std::vector<int64_t>& vec);
    std::vector<int64_t> DeserializeInt64Vector(const std::string& data);
};

} // namespace cache
} // namespace mpim
