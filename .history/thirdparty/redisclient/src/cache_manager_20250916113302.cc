#include "cache_manager.h"
#include "logger/logger.h"
#include <sstream>
#include <cstring>

namespace mpim {
namespace redis {

CacheManager::CacheManager() 
    : redis_client_(&RedisClient::GetInstance())
    , context_(nullptr) {
}

CacheManager::~CacheManager() {
    Disconnect();
}

bool CacheManager::Connect() {
    if (context_ != nullptr) {
        return true;
    }
    
    context_ = redisConnect("127.0.0.1", 6379);
    if (context_ == nullptr || context_->err) {
        if (context_) {
            LOG_ERROR << "CacheManager connection error: " << context_->errstr;
            redisFree(context_);
            context_ = nullptr;
        }
        return false;
    }
    
    LOG_INFO << "CacheManager connected successfully";
    return true;
}

void CacheManager::Disconnect() {
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
    }
}

bool CacheManager::IsConnected() const {
    return context_ != nullptr && !context_->err;
}

// 字符串操作
bool CacheManager::Setex(const std::string& key, int ttl, const std::string& value) {
    if (!IsConnected()) return false;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "SETEX %s %d %b", 
                                                 key.c_str(), ttl, value.c_str(), value.size());
    if (reply == nullptr) return false;
    
    bool result = (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0);
    freeReplyObject(reply);
    return result;
}

std::string CacheManager::Get(const std::string& key, bool* found) {
    if (!IsConnected()) {
        if (found) *found = false;
        return "";
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "GET %s", key.c_str());
    if (reply == nullptr) {
        if (found) *found = false;
        return "";
    }
    
    if (reply->type == REDIS_REPLY_STRING) {
        if (found) *found = true;
        std::string result(reply->str, reply->len);
        freeReplyObject(reply);
        return result;
    } else if (reply->type == REDIS_REPLY_NIL) {
        if (found) *found = false;
        freeReplyObject(reply);
        return "";
    } else {
        if (found) *found = false;
        freeReplyObject(reply);
        return "";
    }
}

bool CacheManager::Del(const std::string& key) {
    if (!IsConnected()) return false;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "DEL %s", key.c_str());
    if (reply == nullptr) return false;
    
    bool result = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return result;
}

bool CacheManager::Exists(const std::string& key) {
    if (!IsConnected()) return false;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "EXISTS %s", key.c_str());
    if (reply == nullptr) return false;
    
    bool result = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return result;
}

// 哈希操作
bool CacheManager::Hset(const std::string& key, const std::string& field, const std::string& value) {
    if (!IsConnected()) return false;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "HSET %s %s %b", 
                                                 key.c_str(), field.c_str(), value.c_str(), value.size());
    if (reply == nullptr) return false;
    
    bool result = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return result;
}

std::string CacheManager::Hget(const std::string& key, const std::string& field) {
    if (!IsConnected()) return "";
    
    redisReply* reply = (redisReply*)redisCommand(context_, "HGET %s %s", key.c_str(), field.c_str());
    if (reply == nullptr) return "";
    
    if (reply->type == REDIS_REPLY_STRING) {
        std::string result(reply->str, reply->len);
        freeReplyObject(reply);
        return result;
    } else if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return "";
    } else {
        freeReplyObject(reply);
        return "";
    }
}

bool CacheManager::Hgetall(const std::string& key, std::map<std::string, std::string>& result) {
    if (!IsConnected()) return false;
    
    result.clear();
    redisReply* reply = (redisReply*)redisCommand(context_, "HGETALL %s", key.c_str());
    if (reply == nullptr) return false;
    
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i += 2) {
            if (i + 1 < reply->elements) {
                std::string field(reply->element[i]->str, reply->element[i]->len);
                std::string value(reply->element[i + 1]->str, reply->element[i + 1]->len);
                result[field] = value;
            }
        }
    }
    
    freeReplyObject(reply);
    return true;
}

bool CacheManager::Hdel(const std::string& key, const std::string& field) {
    if (!IsConnected()) return false;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "HDEL %s %s", key.c_str(), field.c_str());
    if (reply == nullptr) return false;
    
    bool result = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return result;
}

bool CacheManager::Hdel(const std::string& key, const std::vector<std::string>& fields) {
    if (!IsConnected() || fields.empty()) return false;
    
    std::ostringstream cmd;
    cmd << "HDEL " << key;
    for (const auto& field : fields) {
        cmd << " " << field;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, cmd.str().c_str());
    if (reply == nullptr) return false;
    
    bool result = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return result;
}

bool CacheManager::Hexists(const std::string& key, const std::string& field) {
    if (!IsConnected()) return false;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "HEXISTS %s %s", key.c_str(), field.c_str());
    if (reply == nullptr) return false;
    
    bool result = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return result;
}

// 集合操作
bool CacheManager::Sadd(const std::string& key, const std::string& member) {
    if (!IsConnected()) return false;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "SADD %s %s", key.c_str(), member.c_str());
    if (reply == nullptr) return false;
    
    bool result = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return result;
}

bool CacheManager::Srem(const std::string& key, const std::string& member) {
    if (!IsConnected()) return false;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "SREM %s %s", key.c_str(), member.c_str());
    if (reply == nullptr) return false;
    
    bool result = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return result;
}

std::vector<std::string> CacheManager::Smembers(const std::string& key) {
    std::vector<std::string> result;
    if (!IsConnected()) return result;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "SMEMBERS %s", key.c_str());
    if (reply == nullptr) return result;
    
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i++) {
            result.emplace_back(reply->element[i]->str, reply->element[i]->len);
        }
    }
    
    freeReplyObject(reply);
    return result;
}

bool CacheManager::Sismember(const std::string& key, const std::string& member) {
    if (!IsConnected()) return false;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "SISMEMBER %s %s", key.c_str(), member.c_str());
    if (reply == nullptr) return false;
    
    bool result = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return result;
}

int CacheManager::Scard(const std::string& key) {
    if (!IsConnected()) return 0;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "SCARD %s", key.c_str());
    if (reply == nullptr) return 0;
    
    int result = (reply->type == REDIS_REPLY_INTEGER) ? reply->integer : 0;
    freeReplyObject(reply);
    return result;
}

// 过期操作
bool CacheManager::Expire(const std::string& key, int seconds) {
    if (!IsConnected()) return false;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "EXPIRE %s %d", key.c_str(), seconds);
    if (reply == nullptr) return false;
    
    bool result = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return result;
}

bool CacheManager::Ttl(const std::string& key, int* ttl) {
    if (!IsConnected()) return false;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "TTL %s", key.c_str());
    if (reply == nullptr) return false;
    
    if (reply->type == REDIS_REPLY_INTEGER) {
        if (ttl) *ttl = reply->integer;
        freeReplyObject(reply);
        return true;
    }
    
    freeReplyObject(reply);
    return false;
}

} // namespace redis
} // namespace mpim
