#include <iostream>
#include "thirdparty/redisclient/redis.hpp"

int main() {
    Redis redis;
    
    std::cout << "Testing Redis setex operation..." << std::endl;
    
    // 连接Redis
    if (!redis.connect()) {
        std::cout << "Failed to connect to Redis" << std::endl;
        return 1;
    }
    
    std::cout << "Connected to Redis successfully" << std::endl;
    
    // 测试setex操作
    std::string key = "test_route:5";
    std::string value = "gateway-1";
    int ttl = 120;
    
    std::cout << "Testing setex: key=" << key << ", value=" << value << ", ttl=" << ttl << std::endl;
    
    bool result = redis.setex(key, ttl, value);
    
    if (result) {
        std::cout << "setex operation succeeded" << std::endl;
        
        // 验证数据
        auto retrieved = redis.get(key);
        if (!retrieved.empty()) {
            std::cout << "Data verification successful: " << retrieved << std::endl;
        } else {
            std::cout << "Data verification failed: no data retrieved" << std::endl;
        }
    } else {
        std::cout << "setex operation failed" << std::endl;
    }
    
    return 0;
}
