#include <hiredis/hiredis.h>
#include <iostream>
#include <string>

int main() {
    std::cout << "Testing hiredis 1.0.0..." << std::endl;
    
    // 检查版本信息
    std::cout << "Hiredis version: " << HIREDIS_MAJOR << "." << HIREDIS_MINOR << "." << HIREDIS_PATCH << std::endl;
    
    // 尝试连接Redis
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
    
    std::cout << "Connected to Redis successfully!" << std::endl;
    
    // 测试基本命令
    redisReply *reply = (redisReply*)redisCommand(c, "PING");
    if (reply == NULL) {
        std::cout << "PING command failed" << std::endl;
        redisFree(c);
        return 1;
    }
    
    std::cout << "PING response: " << reply->str << std::endl;
    freeReplyObject(reply);
    
    // 测试SET命令
    reply = (redisReply*)redisCommand(c, "SET test_key test_value");
    if (reply == NULL) {
        std::cout << "SET command failed" << std::endl;
        redisFree(c);
        return 1;
    }
    
    std::cout << "SET response: " << reply->str << std::endl;
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
    
    redisFree(c);
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}
