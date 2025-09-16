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

std::string UserCache::GetUserInfo(int64_t user_id) {
    if (!connected_) return "";
    
    std::string key = UserInfoKey(user_id);
    return cache_manager_.Get(key);
}

bool UserCache::SetUserInfo(int64_t user_id, const std::string& user_info, int ttl) {
    if (!connected_) return false;
    
    std::string key = UserInfoKey(user_id);
    return cache_manager_.Setex(key, ttl, user_info);
}

bool UserCache::DelUserInfo(int64_t user_id) {
    if (!connected_) return false;
    
    std::string key = UserInfoKey(user_id);
    return cache_manager_.Del(key);
}

std::vector<int64_t> UserCache::GetFriends(int64_t user_id) {
    if (!connected_) return {};
    
    std::string key = FriendsKey(user_id);
    auto members = cache_manager_.Smembers(key);
    
    std::vector<int64_t> result;
    for (const auto& member : members) {
        try {
            result.push_back(std::stoll(member));
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to parse friend ID: " << member;
        }
    }
    
    return result;
}

bool UserCache::SetFriends(int64_t user_id, const std::vector<int64_t>& friends, int ttl) {
    if (!connected_) return false;
    
    std::string key = FriendsKey(user_id);
    
    // 先删除现有的好友列表
    cache_manager_.Del(key);
    
    // 添加新的好友列表
    for (int64_t friend_id : friends) {
        cache_manager_.Sadd(key, std::to_string(friend_id));
    }
    
    // 设置过期时间
    return cache_manager_.Setex(key, ttl, "");
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

std::vector<int64_t> UserCache::GetGroupMembers(int64_t group_id) {
    if (!connected_) return {};
    
    std::string key = GroupMembersKey(group_id);
    auto members = cache_manager_.Smembers(key);
    
    std::vector<int64_t> result;
    for (const auto& member : members) {
        try {
            result.push_back(std::stoll(member));
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to parse group member ID: " << member;
        }
    }
    
    return result;
}

bool UserCache::SetGroupMembers(int64_t group_id, const std::vector<int64_t>& members, int ttl) {
    if (!connected_) return false;
    
    std::string key = GroupMembersKey(group_id);
    
    // 先删除现有的成员列表
    cache_manager_.Del(key);
    
    // 添加新的成员列表
    for (int64_t member_id : members) {
        cache_manager_.Sadd(key, std::to_string(member_id));
    }
    
    // 设置过期时间
    return cache_manager_.Setex(key, ttl, "");
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

std::string UserCache::UserInfoKey(int64_t user_id) {
    return "user_info:" + std::to_string(user_id);
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

std::string UserCache::SerializeInt64Vector(const std::vector<int64_t>& vec) {
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) oss << ",";
        oss << vec[i];
    }
    return oss.str();
}

std::vector<int64_t> UserCache::DeserializeInt64Vector(const std::string& data) {
    std::vector<int64_t> result;
    std::istringstream iss(data);
    std::string item;
    
    while (std::getline(iss, item, ',')) {
        try {
            result.push_back(std::stoll(item));
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to parse ID: " << item;
        }
    }
    
    return result;
}

} // namespace cache
} // namespace mpim