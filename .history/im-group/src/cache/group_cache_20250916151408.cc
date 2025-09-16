#include "cache/group_cache.h"
#include "logger/logger.h"

namespace mpim {
namespace group {

GroupCache::GroupCache() : connected_(false) {
}

GroupCache::~GroupCache() {
    Disconnect();
}

bool GroupCache::Connect(const std::string& ip, int port) {
    if (connected_) {
        return true;
    }
    
    if (cache_manager_.Connect(ip, port)) {
        connected_ = true;
        LOG_INFO << "GroupCache connected successfully";
        return true;
    }
    
    LOG_WARN << "GroupCache connection failed, will use degraded mode";
    return false;
}

void GroupCache::Disconnect() {
    if (connected_) {
        cache_manager_.Disconnect();
        connected_ = false;
        LOG_INFO << "GroupCache disconnected";
    }
}

bool GroupCache::IsConnected() const {
    return connected_;
}

std::string GroupCache::GetGroupInfo(int64_t group_id) {
    if (!connected_) {
        if (degraded_callback_) {
            return degraded_callback_(GroupInfoKey(group_id));
        }
        return "";
    }
    
    std::string key = GroupInfoKey(group_id);
    return cache_manager_.Get(key);
}

bool GroupCache::SetGroupInfo(int64_t group_id, const std::string& group_data, int ttl) {
    if (!connected_) {
        return false;
    }
    
    std::string key = GroupInfoKey(group_id);
    return cache_manager_.Setex(key, ttl, group_data);
}

bool GroupCache::DelGroupInfo(int64_t group_id) {
    if (!connected_) {
        return false;
    }
    
    std::string key = GroupInfoKey(group_id);
    return cache_manager_.Del(key);
}

std::string GroupCache::GetGroupMembers(int64_t group_id) {
    if (!connected_) {
        if (degraded_callback_) {
            return degraded_callback_(GroupMembersKey(group_id));
        }
        return "";
    }
    
    std::string key = GroupMembersKey(group_id);
    return cache_manager_.Get(key);
}

bool GroupCache::SetGroupMembers(int64_t group_id, const std::string& members_data, int ttl) {
    if (!connected_) {
        return false;
    }
    
    std::string key = GroupMembersKey(group_id);
    return cache_manager_.Setex(key, ttl, members_data);
}

bool GroupCache::DelGroupMembers(int64_t group_id) {
    if (!connected_) {
        return false;
    }
    
    std::string key = GroupMembersKey(group_id);
    return cache_manager_.Del(key);
}

std::string GroupCache::GetUserGroups(int64_t user_id) {
    if (!connected_) {
        if (degraded_callback_) {
            return degraded_callback_(UserGroupsKey(user_id));
        }
        return "";
    }
    
    std::string key = UserGroupsKey(user_id);
    return cache_manager_.Get(key);
}

bool GroupCache::SetUserGroups(int64_t user_id, const std::string& groups_data, int ttl) {
    if (!connected_) {
        return false;
    }
    
    std::string key = UserGroupsKey(user_id);
    return cache_manager_.Setex(key, ttl, groups_data);
}

bool GroupCache::DelUserGroups(int64_t user_id) {
    if (!connected_) {
        return false;
    }
    
    std::string key = UserGroupsKey(user_id);
    return cache_manager_.Del(key);
}

void GroupCache::SetDegradedCallback(std::function<std::string(const std::string&)> callback) {
    degraded_callback_ = callback;
}

void GroupCache::LogMetrics() {
    LOG_INFO << "GroupCache metrics logged";
}

std::string GroupCache::GroupInfoKey(int64_t group_id) {
    return "group:" + std::to_string(group_id);
}

std::string GroupCache::GroupMembersKey(int64_t group_id) {
    return "group_members:" + std::to_string(group_id);
}

std::string GroupCache::UserGroupsKey(int64_t user_id) {
    return "user_groups:" + std::to_string(user_id);
}

} // namespace group
} // namespace mpim
