#pragma once
#include "redis_client.h"
#include <string>
#include <map>
#include <vector>
#include <mutex>

namespace mpim {
namespace redis {

// 缓存管理功能
class CacheManager {
public:
    CacheManager();
    ~CacheManager();
    
    bool Connect(const std::string& ip = "127.0.0.1", int port = 6379);
    void Disconnect();
    bool IsConnected() const;
    
    // 字符串操作
    bool Setex(const std::string& key, int ttl, const std::string& value);
    std::string Get(const std::string& key, bool* found = nullptr);
    bool Del(const std::string& key);
    bool Exists(const std::string& key);
    
    // 哈希操作
    bool Hset(const std::string& key, const std::string& field, const std::string& value);
    std::string Hget(const std::string& key, const std::string& field);
    bool Hgetall(const std::string& key, std::map<std::string, std::string>& result);
    bool Hdel(const std::string& key, const std::string& field);
    bool Hdel(const std::string& key, const std::vector<std::string>& fields);
    bool Hexists(const std::string& key, const std::string& field);
    
    // 集合操作
    bool Sadd(const std::string& key, const std::string& member);
    bool Srem(const std::string& key, const std::string& member);
    std::vector<std::string> Smembers(const std::string& key);
    bool Sismember(const std::string& key, const std::string& member);
    int Scard(const std::string& key);
    
    // 过期操作
    bool Expire(const std::string& key, int seconds);
    bool Ttl(const std::string& key, int* ttl);
    
private:
    RedisClient* redis_client_;
    redisContext* context_;
    mutable std::mutex mu_; // hiredis redisContext 非线程安全，用互斥保护
};

} // namespace redis
} // namespace mpim
