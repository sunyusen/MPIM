#include "cache/user_cache.h"
#include "logger/logger.h"
#include <sstream>

namespace mpim {
namespace cache {

// LoginInfoCache 实现
LoginInfoCache::LoginInfoCache() 
    : GenericCache<mpim::LoginResp>("login_info", CacheConfig{}) {
}

std::string LoginInfoCache::Serialize(const mpim::LoginResp& value) {
    return value.SerializeAsString();
}

std::shared_ptr<mpim::LoginResp> LoginInfoCache::Deserialize(const std::string& data) {
    auto result = std::make_shared<mpim::LoginResp>();
    if (result->ParseFromString(data)) {
        return result;
    }
    return nullptr;
}

// FriendsCache 实现
FriendsCache::FriendsCache() 
    : GenericCache<mpim::GetFriendsResp>("friends", CacheConfig{}) {
}

std::string FriendsCache::Serialize(const mpim::GetFriendsResp& value) {
    return value.SerializeAsString();
}

std::shared_ptr<mpim::GetFriendsResp> FriendsCache::Deserialize(const std::string& data) {
    auto result = std::make_shared<mpim::GetFriendsResp>();
    if (result->ParseFromString(data)) {
        return result;
    }
    return nullptr;
}

bool FriendsCache::AddFriend(int64_t user_id, int64_t friend_id) {
    std::string key = std::to_string(user_id);
    return GenericCache<mpim::GetFriendsResp>::Set(key + ":add", mpim::GetFriendsResp{}, 0);
}

bool FriendsCache::RemoveFriend(int64_t user_id, int64_t friend_id) {
    std::string key = std::to_string(user_id);
    return GenericCache<mpim::GetFriendsResp>::Set(key + ":remove", mpim::GetFriendsResp{}, 0);
}

// GroupInfoCache 实现
GroupInfoCache::GroupInfoCache() 
    : GenericCache<mpim::GroupInfo>("group_info", CacheConfig{}) {
}

std::string GroupInfoCache::Serialize(const mpim::GroupInfo& value) {
    return value.SerializeAsString();
}

std::shared_ptr<mpim::GroupInfo> GroupInfoCache::Deserialize(const std::string& data) {
    auto result = std::make_shared<mpim::GroupInfo>();
    if (result->ParseFromString(data)) {
        return result;
    }
    return nullptr;
}

// GroupMembersCache 实现
GroupMembersCache::GroupMembersCache() 
    : GenericCache<mpim::GetGroupMembersResp>("group_members", CacheConfig{}) {
}

std::string GroupMembersCache::Serialize(const mpim::GetGroupMembersResp& value) {
    return value.SerializeAsString();
}

std::shared_ptr<mpim::GetGroupMembersResp> GroupMembersCache::Deserialize(const std::string& data) {
    auto result = std::make_shared<mpim::GetGroupMembersResp>();
    if (result->ParseFromString(data)) {
        return result;
    }
    return nullptr;
}

bool GroupMembersCache::AddGroupMember(int64_t group_id, int64_t user_id) {
    std::string key = std::to_string(group_id);
    return GenericCache<mpim::GetGroupMembersResp>::Set(key + ":add", mpim::GetGroupMembersResp{}, 0);
}

bool GroupMembersCache::RemoveGroupMember(int64_t group_id, int64_t user_id) {
    std::string key = std::to_string(group_id);
    return GenericCache<mpim::GetGroupMembersResp>::Set(key + ":remove", mpim::GetGroupMembersResp{}, 0);
}

// UserGroupsCache 实现
UserGroupsCache::UserGroupsCache() 
    : GenericCache<mpim::GetUserGroupsResp>("user_groups", CacheConfig{}) {
}

std::string UserGroupsCache::Serialize(const mpim::GetUserGroupsResp& value) {
    return value.SerializeAsString();
}

std::shared_ptr<mpim::GetUserGroupsResp> UserGroupsCache::Deserialize(const std::string& data) {
    auto result = std::make_shared<mpim::GetUserGroupsResp>();
    if (result->ParseFromString(data)) {
        return result;
    }
    return nullptr;
}

// UserCacheManager 实现
UserCacheManager::UserCacheManager() : connected_(false) {
}

UserCacheManager::~UserCacheManager() {
    Disconnect();
}

bool UserCacheManager::Connect(const std::string& ip, int port) {
    if (connected_) {
        return true;
    }
    
    bool success = true;
    success &= login_cache_.Connect(ip, port);
    success &= friends_cache_.Connect(ip, port);
    success &= group_info_cache_.Connect(ip, port);
    success &= group_members_cache_.Connect(ip, port);
    success &= user_groups_cache_.Connect(ip, port);
    
    if (success) {
        connected_ = true;
        LOG_INFO << "UserCacheManager connected successfully";
    } else {
        LOG_ERROR << "UserCacheManager connection failed";
    }
    
    return success;
}

void UserCacheManager::Disconnect() {
    if (connected_) {
        login_cache_.Disconnect();
        friends_cache_.Disconnect();
        group_info_cache_.Disconnect();
        group_members_cache_.Disconnect();
        user_groups_cache_.Disconnect();
        connected_ = false;
        LOG_INFO << "UserCacheManager disconnected";
    }
}

bool UserCacheManager::IsConnected() const {
    return connected_;
}

std::shared_ptr<mpim::LoginResp> UserCacheManager::GetLoginInfo(int64_t user_id) {
    return login_cache_.Get(LoginInfoKey(user_id));
}

bool UserCacheManager::SetLoginInfo(int64_t user_id, const mpim::LoginResp& login_resp, int ttl) {
    return login_cache_.Set(LoginInfoKey(user_id), login_resp, ttl);
}

bool UserCacheManager::DelLoginInfo(int64_t user_id) {
    return login_cache_.Del(LoginInfoKey(user_id));
}

std::shared_ptr<mpim::GetFriendsResp> UserCacheManager::GetFriends(int64_t user_id) {
    return friends_cache_.Get(FriendsKey(user_id));
}

bool UserCacheManager::SetFriends(int64_t user_id, const mpim::GetFriendsResp& friends_resp, int ttl) {
    return friends_cache_.Set(FriendsKey(user_id), friends_resp, ttl);
}

bool UserCacheManager::AddFriend(int64_t user_id, int64_t friend_id) {
    return friends_cache_.AddFriend(user_id, friend_id);
}

bool UserCacheManager::RemoveFriend(int64_t user_id, int64_t friend_id) {
    return friends_cache_.RemoveFriend(user_id, friend_id);
}

std::shared_ptr<mpim::GroupInfo> UserCacheManager::GetGroupInfo(int64_t group_id) {
    return group_info_cache_.Get(GroupInfoKey(group_id));
}

bool UserCacheManager::SetGroupInfo(int64_t group_id, const mpim::GroupInfo& group_info, int ttl) {
    return group_info_cache_.Set(GroupInfoKey(group_id), group_info, ttl);
}

bool UserCacheManager::DelGroupInfo(int64_t group_id) {
    return group_info_cache_.Del(GroupInfoKey(group_id));
}

std::shared_ptr<mpim::GetGroupMembersResp> UserCacheManager::GetGroupMembers(int64_t group_id) {
    return group_members_cache_.Get(GroupMembersKey(group_id));
}

bool UserCacheManager::SetGroupMembers(int64_t group_id, const mpim::GetGroupMembersResp& members_resp, int ttl) {
    return group_members_cache_.Set(GroupMembersKey(group_id), members_resp, ttl);
}

bool UserCacheManager::AddGroupMember(int64_t group_id, int64_t user_id) {
    return group_members_cache_.AddGroupMember(group_id, user_id);
}

bool UserCacheManager::RemoveGroupMember(int64_t group_id, int64_t user_id) {
    return group_members_cache_.RemoveGroupMember(group_id, user_id);
}

std::shared_ptr<mpim::GetUserGroupsResp> UserCacheManager::GetUserGroups(int64_t user_id) {
    return user_groups_cache_.Get(UserGroupsKey(user_id));
}

bool UserCacheManager::SetUserGroups(int64_t user_id, const mpim::GetUserGroupsResp& groups_resp, int ttl) {
    return user_groups_cache_.Set(UserGroupsKey(user_id), groups_resp, ttl);
}

void UserCacheManager::SetDegradedCallbacks(
    std::function<std::shared_ptr<mpim::LoginResp>(int64_t)> login_callback,
    std::function<std::shared_ptr<mpim::GetFriendsResp>(int64_t)> friends_callback,
    std::function<std::shared_ptr<mpim::GroupInfo>(int64_t)> group_info_callback,
    std::function<std::shared_ptr<mpim::GetGroupMembersResp>(int64_t)> group_members_callback,
    std::function<std::shared_ptr<mpim::GetUserGroupsResp>(int64_t)> user_groups_callback
) {
    login_cache_.SetDegradedCallback([login_callback](const std::string& key) {
        // 从key中提取user_id
        int64_t user_id = std::stoll(key);
        return login_callback(user_id);
    });
    
    friends_cache_.SetDegradedCallback([friends_callback](const std::string& key) {
        int64_t user_id = std::stoll(key);
        return friends_callback(user_id);
    });
    
    group_info_cache_.SetDegradedCallback([group_info_callback](const std::string& key) {
        int64_t group_id = std::stoll(key);
        return group_info_callback(group_id);
    });
    
    group_members_cache_.SetDegradedCallback([group_members_callback](const std::string& key) {
        int64_t group_id = std::stoll(key);
        return group_members_callback(group_id);
    });
    
    user_groups_cache_.SetDegradedCallback([user_groups_callback](const std::string& key) {
        int64_t user_id = std::stoll(key);
        return user_groups_callback(user_id);
    });
}

void UserCacheManager::LogMetrics() {
    LOG_INFO << "UserCacheManager metrics:";
    login_cache_.LogMetrics();
    friends_cache_.LogMetrics();
    group_info_cache_.LogMetrics();
    group_members_cache_.LogMetrics();
    user_groups_cache_.LogMetrics();
}

std::string UserCacheManager::LoginInfoKey(int64_t user_id) {
    return std::to_string(user_id);
}

std::string UserCacheManager::FriendsKey(int64_t user_id) {
    return std::to_string(user_id);
}

std::string UserCacheManager::GroupInfoKey(int64_t group_id) {
    return std::to_string(group_id);
}

std::string UserCacheManager::GroupMembersKey(int64_t group_id) {
    return std::to_string(group_id);
}

std::string UserCacheManager::UserGroupsKey(int64_t user_id) {
    return std::to_string(user_id);
}

} // namespace cache
} // namespace mpim
