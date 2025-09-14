#include <hiredis/hiredis.h>
#include <iostream>

int main() {
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        std::cout << "Connection error" << std::endl;
        return 1;
    }
    
    // 测试简单的PING命令
    redisReply *reply = (redisReply*)redisCommand(c, "PING");
    if (reply) {
        std::cout << "PING reply type: " << reply->type << std::endl;
        if (reply->str) {
            std::cout << "PING reply: " << reply->str << std::endl;
        }
        freeReplyObject(reply);
    }
    
    // 测试SET命令
    reply = (redisReply*)redisCommand(c, "SET test_key test_value");
    if (reply) {
        std::cout << "SET reply type: " << reply->type << std::endl;
        if (reply->str) {
            std::cout << "SET reply: " << reply->str << std::endl;
        }
        freeReplyObject(reply);
    }
    
    // 测试GET命令
    reply = (redisReply*)redisCommand(c, "GET test_key");
    if (reply) {
        std::cout << "GET reply type: " << reply->type << std::endl;
        if (reply->str) {
            std::cout << "GET reply: " << reply->str << std::endl;
        }
        freeReplyObject(reply);
    }
    
    // 测试SETEX命令
    reply = (redisReply*)redisCommand(c, "SETEX test_key2 60 test_value2");
    if (reply) {
        std::cout << "SETEX reply type: " << reply->type << std::endl;
        if (reply->str) {
            std::cout << "SETEX reply: " << reply->str << std::endl;
        }
        freeReplyObject(reply);
    }
    
    redisFree(c);
    return 0;
}
