#include <hiredis/hiredis.h>
#include <iostream>
#include <string>

int main() {
    std::cout << "Testing Redis connection..." << std::endl;
    
    // 测试基本连接
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
    
    // 测试PING命令
    redisReply *reply = (redisReply*)redisCommand(c, "PING");
    if (reply == NULL) {
        std::cout << "PING command failed" << std::endl;
        redisFree(c);
        return 1;
    }
    
    std::cout << "PING response: " << reply->str << std::endl;
    freeReplyObject(reply);
    
    // 测试SETEX命令
    reply = (redisReply*)redisCommand(c, "SETEX test_key 60 test_value");
    if (reply == NULL) {
        std::cout << "SETEX command failed" << std::endl;
        redisFree(c);
        return 1;
    }
    
    std::cout << "SETEX response: " << reply->str << std::endl;
    freeReplyObject(reply);
    
    // 测试GET命令
    reply = (redisReply*)redisCommand(c, "GET test_key");
    if (reply == NULL) {
        std::cout << "GET command failed" << std::endl;
        redisFree(c);
        return 1;
    }
    
    if (reply->type == REDIS_REPLY_STRING) {
        std::cout << "GET response: " << reply->str << std::endl;
    } else {
        std::cout << "GET response type: " << reply->type << std::endl;
    }
    freeReplyObject(reply);
    
    // 清理测试键
    reply = (redisReply*)redisCommand(c, "DEL test_key");
    freeReplyObject(reply);
    
    redisFree(c);
    std::cout << "Redis test completed successfully!" << std::endl;
    return 0;
}
