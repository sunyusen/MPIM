#include "cache/user_cache.h"
#include "logger/logger.h"
#include <sstream>

namespace mpim {
namespace cache {

UserCache::UserCache() : connected_(false) {}

UserCache::~UserCache() {}

bool UserCache::Connect() {
    if (connected_) {
        return true;
    }
    
    if (cache_manager_.Connect()) {
        connected_ = true;
        LOG_INFO << "UserCache connected successfully";
        return true;
    }
    
    return false;
}

void UserCache::Disconnect() {
    connected_ = false;
}

std::shared_ptr<mpim::LoginResp> UserCache::GetLoginInfo(int64_t user_id) {
    if (!connected_) return nullptr;
    
    std::string key = LoginInfoKey(user_id);
    std::string data = cache_manager_.Get(key);
    
    if (data.empty()) {
        return nullptr;
    }
    
    return DeserializeLoginResp(data);
}

bool UserCache::SetLoginInfo(int64_t user_id, const mpim::LoginResp& login_resp, int ttl) {
    if (!connected_) return false;
    
    std::string key = LoginInfoKey(user_id);
    std::string data = SerializeLoginResp(login_resp);
    
    return cache_manager_.Setex(key, ttl, data);
}

bool UserCache::DelLoginInfo(int64_t user_id) {
    if (!connected_) return false;
    
    std::string key = LoginInfoKey(user_id);
    return cache_manager_.Del(key);
}

std::shared_ptr<mpim::GetFriendsResp> UserCache::GetFriends(int64_t user_id) {
    if (!connected_) return nullptr;
    
    std::string key = FriendsKey(user_id);
    std::string data = cache_manager_.Get(key);
    
    if (data.empty()) {
        return nullptr;
    }
    
    return DeserializeGetFriendsResp(data);
}

bool UserCache::SetFriends(int64_t user_id, const mpim::GetFriendsResp& friends_resp, int ttl) {
    if (!connected_) return false;
    
    std::string key = FriendsKey(user_id);
    std::string data = SerializeGetFriendsResp(friends_resp);
    
    return cache_manager_.Setex(key, ttl, data);
}

bool UserCache::AddFriend(int64_t user_id, int64_t friend_id) {
    if (!connected_) return false;
    
    std::string key = FriendsKey(user_id);
    return cache_manager_.Sadd(key, std::to_string(friend_id));
}

bool UserCache::RemoveFriend(int64_t user_id, int64_t friend_id) {
    if (!connected_) return false;
    
    std::string key = FriendsKey(user_id);
    return cache_manager_.Srem(key, std::to_string(friend_id));
}

std::shared_ptr<mpim::GroupInfo> UserCache::GetGroupInfo(int64_t group_id) {
    if (!connected_) return nullptr;
    
    std::string key = GroupInfoKey(group_id);
    std::string data = cache_manager_.Get(key);
    
    if (data.empty()) {
        return nullptr;
    }
    
    return DeserializeGroupInfo(data);
}

bool UserCache::SetGroupInfo(int64_t group_id, const mpim::GroupInfo& group_info, int ttl) {
    if (!connected_) return false;
    
    std::string key = GroupInfoKey(group_id);
    std::string data = SerializeGroupInfo(group_info);
    
    return cache_manager_.Setex(key, ttl, data);
}

bool UserCache::DelGroupInfo(int64_t group_id) {
    if (!connected_) return false;
    
    std::string key = GroupInfoKey(group_id);
    return cache_manager_.Del(key);
}

std::shared_ptr<mpim::GetGroupMembersResp> UserCache::GetGroupMembers(int64_t group_id) {
    if (!connected_) return nullptr;
    
    std::string key = GroupMembersKey(group_id);
    std::string data = cache_manager_.Get(key);
    
    if (data.empty()) {
        return nullptr;
    }
    
    return DeserializeGetGroupMembersResp(data);
}

bool UserCache::SetGroupMembers(int64_t group_id, const mpim::GetGroupMembersResp& members_resp, int ttl) {
    if (!connected_) return false;
    
    std::string key = GroupMembersKey(group_id);
    std::string data = SerializeGetGroupMembersResp(members_resp);
    
    return cache_manager_.Setex(key, ttl, data);
}

bool UserCache::AddGroupMember(int64_t group_id, int64_t user_id) {
    if (!connected_) return false;
    
    std::string key = GroupMembersKey(group_id);
    return cache_manager_.Sadd(key, std::to_string(user_id));
}

bool UserCache::RemoveGroupMember(int64_t group_id, int64_t user_id) {
    if (!connected_) return false;
    
    std::string key = GroupMembersKey(group_id);
    return cache_manager_.Srem(key, std::to_string(user_id));
}

std::shared_ptr<mpim::GetUserGroupsResp> UserCache::GetUserGroups(int64_t user_id) {
    if (!connected_) return nullptr;
    
    std::string key = UserGroupsKey(user_id);
    std::string data = cache_manager_.Get(key);
    
    if (data.empty()) {
        return nullptr;
    }
    
    return DeserializeGetUserGroupsResp(data);
}

bool UserCache::SetUserGroups(int64_t user_id, const mpim::GetUserGroupsResp& groups_resp, int ttl) {
    if (!connected_) return false;
    
    std::string key = UserGroupsKey(user_id);
    std::string data = SerializeGetUserGroupsResp(groups_resp);
    
    return cache_manager_.Setex(key, ttl, data);
}

std::string UserCache::LoginInfoKey(int64_t user_id) {
    return "login_info:" + std::to_string(user_id);
}

std::string UserCache::FriendsKey(int64_t user_id) {
    return "friends:" + std::to_string(user_id);
}

std::string UserCache::GroupInfoKey(int64_t group_id) {
    return "group_info:" + std::to_string(group_id);
}

std::string UserCache::GroupMembersKey(int64_t group_id) {
    return "group_members:" + std::to_string(group_id);
}

std::string UserCache::UserGroupsKey(int64_t user_id) {
    return "user_groups:" + std::to_string(user_id);
}

std::string UserCache::SerializeLoginResp(const mpim::LoginResp& login_resp) {
    return login_resp.SerializeAsString();
}

std::shared_ptr<mpim::LoginResp> UserCache::DeserializeLoginResp(const std::string& data) {
    auto login_resp = std::make_shared<mpim::LoginResp>();
    if (login_resp->ParseFromString(data)) {
        return login_resp;
    }
    return nullptr;
}

std::string UserCache::SerializeGetFriendsResp(const mpim::GetFriendsResp& friends_resp) {
    return friends_resp.SerializeAsString();
}

std::shared_ptr<mpim::GetFriendsResp> UserCache::DeserializeGetFriendsResp(const std::string& data) {
    auto friends_resp = std::make_shared<mpim::GetFriendsResp>();
    if (friends_resp->ParseFromString(data)) {
        return friends_resp;
    }
    return nullptr;
}

std::string UserCache::SerializeGroupInfo(const mpim::GroupInfo& group_info) {
    return group_info.SerializeAsString();
}

std::shared_ptr<mpim::GroupInfo> UserCache::DeserializeGroupInfo(const std::string& data) {
    auto group_info = std::make_shared<mpim::GroupInfo>();
    if (group_info->ParseFromString(data)) {
        return group_info;
    }
    return nullptr;
}

std::string UserCache::SerializeGetGroupMembersResp(const mpim::GetGroupMembersResp& members_resp) {
    return members_resp.SerializeAsString();
}

std::shared_ptr<mpim::GetGroupMembersResp> UserCache::DeserializeGetGroupMembersResp(const std::string& data) {
    auto members_resp = std::make_shared<mpim::GetGroupMembersResp>();
    if (members_resp->ParseFromString(data)) {
        return members_resp;
    }
    return nullptr;
}

std::string UserCache::SerializeGetUserGroupsResp(const mpim::GetUserGroupsResp& groups_resp) {
    return groups_resp.SerializeAsString();
}

std::shared_ptr<mpim::GetUserGroupsResp> UserCache::DeserializeGetUserGroupsResp(const std::string& data) {
    auto groups_resp = std::make_shared<mpim::GetUserGroupsResp>();
    if (groups_resp->ParseFromString(data)) {
        return groups_resp;
    }
    return nullptr;
}

} // namespace cache
} // namespace mpim