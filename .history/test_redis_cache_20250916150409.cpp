#include <iostream>
#include <string>
#include "im-common/include/cache/business_cache.h"

int main() {
    std::cout << "Testing Redis as MySQL cache layer..." << std::endl;
    
    // 创建缓存实例
    mpim::cache::CacheConfig config;
    config.enable_cache = true;
    config.enable_degraded_mode = true;
    config.default_ttl = 3600;
    
    mpim::cache::BusinessCacheImpl cache("test", config);
    
    // 测试连接
    std::cout << "Connecting to Redis..." << std::endl;
    if (cache.Connect()) {
        std::cout << "✓ Redis connected successfully" << std::endl;
    } else {
        std::cout << "✗ Redis connection failed, but degraded mode enabled" << std::endl;
    }
    
    // 测试缓存操作
    std::cout << "\nTesting cache operations..." << std::endl;
    
    // 设置缓存
    std::string key = "user:testuser";
    std::string value = "12345:testuser";
    if (cache.Set(key, value, 60)) {
        std::cout << "✓ Cache SET operation successful" << std::endl;
    } else {
        std::cout << "✗ Cache SET operation failed" << std::endl;
    }
    
    // 获取缓存
    std::string cached_value = cache.Get(key);
    if (!cached_value.empty()) {
        std::cout << "✓ Cache GET operation successful: " << cached_value << std::endl;
    } else {
        std::cout << "✗ Cache GET operation failed or empty" << std::endl;
    }
    
    // 测试降级模式
    std::cout << "\nTesting degraded mode..." << std::endl;
    cache.SetDegradedCallback([](const std::string& key) -> std::string {
        std::cout << "Degraded callback called for key: " << key << std::endl;
        return "degraded_value";
    });
    
    std::string degraded_value = cache.Get("nonexistent_key");
    std::cout << "Degraded value: " << degraded_value << std::endl;
    
    // 显示统计信息
    std::cout << "\nCache statistics:" << std::endl;
    std::cout << "Hits: " << cache.GetCacheHits() << std::endl;
    std::cout << "Misses: " << cache.GetCacheMisses() << std::endl;
    std::cout << "Degraded requests: " << cache.GetDegradedRequests() << std::endl;
    
    cache.Disconnect();
    std::cout << "\nTest completed!" << std::endl;
    
    return 0;
}
