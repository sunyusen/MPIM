#pragma once
#include "cache_manager.h"
#include "user.pb.h"
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
    
    // 用户登录信息缓存
    std::shared_ptr<mpim::LoginResp> GetLoginInfo(int64_t user_id);
    bool SetLoginInfo(int64_t user_id, const mpim::LoginResp& login_resp, int ttl = 3600);
    bool DelLoginInfo(int64_t user_id);
    
    // 好友关系缓存
    std::shared_ptr<mpim::GetFriendsResp> GetFriends(int64_t user_id);
    bool SetFriends(int64_t user_id, const mpim::GetFriendsResp& friends_resp, int ttl = 1800);
    bool AddFriend(int64_t user_id, int64_t friend_id);
    bool RemoveFriend(int64_t user_id, int64_t friend_id);
    
    // 群组信息缓存
    std::shared_ptr<mpim::GroupInfo> GetGroupInfo(int64_t group_id);
    bool SetGroupInfo(int64_t group_id, const mpim::GroupInfo& group_info, int ttl = 3600);
    bool DelGroupInfo(int64_t group_id);
    
    // 群组成员缓存
    std::shared_ptr<mpim::GetGroupMembersResp> GetGroupMembers(int64_t group_id);
    bool SetGroupMembers(int64_t group_id, const mpim::GetGroupMembersResp& members_resp, int ttl = 1800);
    bool AddGroupMember(int64_t group_id, int64_t user_id);
    bool RemoveGroupMember(int64_t group_id, int64_t user_id);
    
    // 用户群组列表缓存
    std::shared_ptr<mpim::GetUserGroupsResp> GetUserGroups(int64_t user_id);
    bool SetUserGroups(int64_t user_id, const mpim::GetUserGroupsResp& groups_resp, int ttl = 1800);
    
private:
    mpim::redis::CacheManager cache_manager_;
    bool connected_;
    
    // 缓存键生成
    std::string LoginInfoKey(int64_t user_id);
    std::string FriendsKey(int64_t user_id);
    std::string GroupInfoKey(int64_t group_id);
    std::string GroupMembersKey(int64_t group_id);
    std::string UserGroupsKey(int64_t user_id);
    
    // 序列化/反序列化
    std::string SerializeLoginResp(const mpim::LoginResp& login_resp);
    std::shared_ptr<mpim::LoginResp> DeserializeLoginResp(const std::string& data);
    std::string SerializeGetFriendsResp(const mpim::GetFriendsResp& friends_resp);
    std::shared_ptr<mpim::GetFriendsResp> DeserializeGetFriendsResp(const std::string& data);
    std::string SerializeGroupInfo(const mpim::GroupInfo& group_info);
    std::shared_ptr<mpim::GroupInfo> DeserializeGroupInfo(const std::string& data);
    std::string SerializeGetGroupMembersResp(const mpim::GetGroupMembersResp& members_resp);
    std::shared_ptr<mpim::GetGroupMembersResp> DeserializeGetGroupMembersResp(const std::string& data);
    std::string SerializeGetUserGroupsResp(const mpim::GetUserGroupsResp& groups_resp);
    std::shared_ptr<mpim::GetUserGroupsResp> DeserializeGetUserGroupsResp(const std::string& data);
};

} // namespace cache
} // namespace mpim
