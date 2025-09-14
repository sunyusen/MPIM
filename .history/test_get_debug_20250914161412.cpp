#include <hiredis/hiredis.h>
#include <iostream>
#include <string>

int main() {
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        std::cout << "Connection error" << std::endl;
        return 1;
    }
    
    // 先设置一个值
    redisReply *reply = (redisReply*)redisCommand(c, "SET test_key test_value");
    if (reply) {
        std::cout << "SET reply type: " << reply->type << std::endl;
        freeReplyObject(reply);
    }
    
    // 然后获取这个值
    reply = (redisReply*)redisCommand(c, "GET test_key");
    if (reply) {
        std::cout << "GET reply type: " << reply->type << std::endl;
        std::cout << "GET reply len: " << reply->len << std::endl;
        std::cout << "GET reply str: " << (reply->str ? reply->str : "NULL") << std::endl;
        std::cout << "GET reply elements: " << reply->elements << std::endl;
        
        if (reply->type == REDIS_REPLY_STRING && reply->str) {
            std::string value(reply->str, reply->len);
            std::cout << "Value: " << value << std::endl;
        }
        
        freeReplyObject(reply);
    }
    
    // 清理
    reply = (redisReply*)redisCommand(c, "DEL test_key");
    freeReplyObject(reply);
    
    redisFree(c);
    return 0;
}
