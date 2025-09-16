#pragma once
#include "business_cache.h"
#include "user.pb.h"
#include "group.pb.h"
#include <string>
#include <memory>
#include <functional>

namespace mpim {
namespace cache {

// 用户缓存管理器 - 使用业务无关的缓存接口
class UserCacheManager {
public:
    UserCacheManager();
    ~UserCacheManager();
    
    // 连接管理
    bool Connect(const std::string& ip = "127.0.0.1", int port = 6379);
    void Disconnect();
    bool IsConnected() const;
    
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
    
    // 设置降级回调
    void SetDegradedCallbacks(
        std::function<std::shared_ptr<mpim::LoginResp>(int64_t)> login_callback,
        std::function<std::shared_ptr<mpim::GetFriendsResp>(int64_t)> friends_callback,
        std::function<std::shared_ptr<mpim::GroupInfo>(int64_t)> group_info_callback,
        std::function<std::shared_ptr<mpim::GetGroupMembersResp>(int64_t)> group_members_callback,
        std::function<std::shared_ptr<mpim::GetUserGroupsResp>(int64_t)> user_groups_callback
    );
    
    // 统计信息
    void LogMetrics();

private:
    // 使用业务无关的缓存接口
    std::unique_ptr<BusinessCache> login_cache_;
    std::unique_ptr<BusinessCache> friends_cache_;
    std::unique_ptr<BusinessCache> group_info_cache_;
    std::unique_ptr<BusinessCache> group_members_cache_;
    std::unique_ptr<BusinessCache> user_groups_cache_;
    
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
