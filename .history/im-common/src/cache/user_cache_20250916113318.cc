#include "cache/user_cache.h"
#include "logger/logger.h"
#include <sstream>

namespace mpim {
namespace cache {

UserCache::UserCache() : connected_(false) {
}

UserCache::~UserCache() {
    Disconnect();
}

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
    cache_manager_.Disconnect();
    connected_ = false;
}

// 用户信息缓存
std::shared_ptr<mpim::User> UserCache::GetUser(int64_t user_id) {
    if (!connected_) return nullptr;
    
    std::string key = UserKey(user_id);
    bool found = false;
    std::string data = cache_manager_.Get(key, &found);
    
    if (!found || data.empty()) {
        return nullptr;
    }
    
    return DeserializeUser(data);
}

bool UserCache::SetUser(int64_t user_id, const mpim::User& user, int ttl) {
    if (!connected_) return false;
    
    std::string key = UserKey(user_id);
    std::string data = SerializeUser(user);
    
    return cache_manager_.Setex(key, ttl, data);
}

bool UserCache::DelUser(int64_t user_id) {
    if (!connected_) return false;
    
    std::string key = UserKey(user_id);
    return cache_manager_.Del(key);
}

// 好友关系缓存
std::vector<int64_t> UserCache::GetFriends(int64_t user_id) {
    if (!connected_) return {};
    
    std::string key = FriendsKey(user_id);
    bool found = false;
    std::string data = cache_manager_.Get(key, &found);
    
    if (!found || data.empty()) {
        return {};
    }
    
    return DeserializeInt64Vector(data);
}

bool UserCache::SetFriends(int64_t user_id, const std::vector<int64_t>& friends, int ttl) {
    if (!connected_) return false;
    
    std::string key = FriendsKey(user_id);
    std::string data = SerializeInt64Vector(friends);
    
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

// 群组信息缓存
std::shared_ptr<mpim::Group> UserCache::GetGroup(int64_t group_id) {
    if (!connected_) return nullptr;
    
    std::string key = GroupKey(group_id);
    bool found = false;
    std::string data = cache_manager_.Get(key, &found);
    
    if (!found || data.empty()) {
        return nullptr;
    }
    
    return DeserializeGroup(data);
}

bool UserCache::SetGroup(int64_t group_id, const mpim::Group& group, int ttl) {
    if (!connected_) return false;
    
    std::string key = GroupKey(group_id);
    std::string data = SerializeGroup(group);
    
    return cache_manager_.Setex(key, ttl, data);
}

bool UserCache::DelGroup(int64_t group_id) {
    if (!connected_) return false;
    
    std::string key = GroupKey(group_id);
    return cache_manager_.Del(key);
}

// 群组成员缓存
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
    
    // 先删除现有成员
    cache_manager_.Del(key);
    
    // 添加新成员
    bool success = true;
    for (int64_t member : members) {
        if (!cache_manager_.Sadd(key, std::to_string(member))) {
            success = false;
        }
    }
    
    // 设置过期时间
    if (success) {
        cache_manager_.Expire(key, ttl);
    }
    
    return success;
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

// 私有方法实现
std::string UserCache::UserKey(int64_t user_id) {
    return "user:" + std::to_string(user_id);
}

std::string UserCache::FriendsKey(int64_t user_id) {
    return "friends:" + std::to_string(user_id);
}

std::string UserCache::GroupKey(int64_t group_id) {
    return "group:" + std::to_string(group_id);
}

std::string UserCache::GroupMembersKey(int64_t group_id) {
    return "group_members:" + std::to_string(group_id);
}

std::string UserCache::SerializeUser(const mpim::User& user) {
    return user.SerializeAsString();
}

std::shared_ptr<mpim::User> UserCache::DeserializeUser(const std::string& data) {
    auto user = std::make_shared<mpim::User>();
    if (user->ParseFromString(data)) {
        return user;
    }
    return nullptr;
}

std::string UserCache::SerializeGroup(const mpim::Group& group) {
    return group.SerializeAsString();
}

std::shared_ptr<mpim::Group> UserCache::DeserializeGroup(const std::string& data) {
    auto group = std::make_shared<mpim::Group>();
    if (group->ParseFromString(data)) {
        return group;
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
            std::cerr << "Failed to parse ID: " << item << std::endl;
        }
    }
    
    return result;
}

} // namespace cache
} // namespace mpim
