#pragma once
#include "cache_manager.h"
#include "user.pb.h"
#include "group.pb.h"
#include <string>
#include <memory>

namespace mpim {
namespace cache {

// 用户信息缓存服务
class UserCache {
public:
    UserCache();
    ~UserCache();
    
    bool Connect();
    void Disconnect();
    
    // 用户信息缓存
    std::shared_ptr<mpim::User> GetUser(int64_t user_id);
    bool SetUser(int64_t user_id, const mpim::User& user, int ttl = 3600);
    bool DelUser(int64_t user_id);
    
    // 好友关系缓存
    std::vector<int64_t> GetFriends(int64_t user_id);
    bool SetFriends(int64_t user_id, const std::vector<int64_t>& friends, int ttl = 1800);
    bool AddFriend(int64_t user_id, int64_t friend_id);
    bool RemoveFriend(int64_t user_id, int64_t friend_id);
    
    // 群组信息缓存
    std::shared_ptr<mpim::Group> GetGroup(int64_t group_id);
    bool SetGroup(int64_t group_id, const mpim::Group& group, int ttl = 3600);
    bool DelGroup(int64_t group_id);
    
    // 群组成员缓存
    std::vector<int64_t> GetGroupMembers(int64_t group_id);
    bool SetGroupMembers(int64_t group_id, const std::vector<int64_t>& members, int ttl = 1800);
    bool AddGroupMember(int64_t group_id, int64_t user_id);
    bool RemoveGroupMember(int64_t group_id, int64_t user_id);
    
private:
    mpim::redis::CacheManager cache_manager_;
    bool connected_;
    
    // 缓存键生成
    std::string UserKey(int64_t user_id);
    std::string FriendsKey(int64_t user_id);
    std::string GroupKey(int64_t group_id);
    std::string GroupMembersKey(int64_t group_id);
    
    // 序列化/反序列化
    std::string SerializeUser(const mpim::User& user);
    std::shared_ptr<mpim::User> DeserializeUser(const std::string& data);
    std::string SerializeGroup(const mpim::Group& group);
    std::shared_ptr<mpim::Group> DeserializeGroup(const std::string& data);
    std::string SerializeInt64Vector(const std::vector<int64_t>& vec);
    std::vector<int64_t> DeserializeInt64Vector(const std::string& data);
};

} // namespace cache
} // namespace mpim
