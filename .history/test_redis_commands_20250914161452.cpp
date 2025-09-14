#include <hiredis/hiredis.h>
#include <iostream>
#include <string>

int main() {
    std::cout << "Testing Redis commands..." << std::endl;
    
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            std::cout << "Connection error: " << c->errstr << std::endl;
            redisFree(c);
        } else {
            std::cout << "Can't allocate redis context" << std::endl;
        }
        return 1;
    }
    
    std::cout << "Redis connection successful!" << std::endl;
    
    // 测试SETEX命令 - 使用二进制格式
    std::cout << "Testing SETEX with binary format..." << std::endl;
    std::string key = "test_key";
    std::string value = "test_value";
    int ttl = 60;
    
    redisReply *reply = (redisReply*)redisCommand(
        c, "SETEX %b %d %b",
        key.data(), (size_t)key.size(),
        ttl,
        value.data(), (size_t)value.size());
    
    if (reply == NULL) {
        std::cout << "SETEX command failed - reply is NULL" << std::endl;
        redisFree(c);
        return 1;
    }
    
    std::cout << "SETEX reply type: " << reply->type << std::endl;
    if (reply->str) {
        std::cout << "SETEX reply: " << reply->str << std::endl;
    }
    freeReplyObject(reply);
    
    // 测试GET命令 - 使用二进制格式
    std::cout << "Testing GET with binary format..." << std::endl;
    reply = (redisReply*)redisCommand(c, "GET %b", key.data(), (size_t)key.size());
    
    if (reply == NULL) {
        std::cout << "GET command failed - reply is NULL" << std::endl;
        redisFree(c);
        return 1;
    }
    
    std::cout << "GET reply type: " << reply->type << std::endl;
    if (reply->type == REDIS_REPLY_STRING) {
        std::cout << "GET reply: " << reply->str << std::endl;
    }
    freeReplyObject(reply);
    
    // 清理
    reply = (redisReply*)redisCommand(c, "DEL %b", key.data(), (size_t)key.size());
    freeReplyObject(reply);
    
    redisFree(c);
    std::cout << "Redis commands test completed!" << std::endl;
    return 0;
}
