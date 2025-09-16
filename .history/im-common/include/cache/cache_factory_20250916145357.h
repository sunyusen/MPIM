#pragma once
#include "generic_cache.h"
#include <memory>
#include <string>
#include <functional>

namespace mpim {
namespace cache {

// 缓存工厂接口
template<typename T>
class CacheFactory {
public:
    virtual ~CacheFactory() = default;
    virtual std::shared_ptr<GenericCache<T>> createCache(const std::string& key_prefix, const CacheConfig& config) = 0;
};

// 默认缓存工厂实现
template<typename T>
class DefaultCacheFactory : public CacheFactory<T> {
public:
    std::shared_ptr<GenericCache<T>> createCache(const std::string& key_prefix, const CacheConfig& config) override {
        return std::make_shared<GenericCache<T>>(key_prefix, config);
    }
};

// 缓存管理器
class CacheManager {
public:
    static CacheManager& getInstance();
    
    // 注册缓存工厂
    template<typename T>
    void registerFactory(const std::string& type, std::shared_ptr<CacheFactory<T>> factory);
    
    // 创建缓存
    template<typename T>
    std::shared_ptr<GenericCache<T>> createCache(const std::string& type, const std::string& key_prefix, const CacheConfig& config = CacheConfig{});
    
    // 全局配置
    void setGlobalConfig(const CacheConfig& config);
    CacheConfig getGlobalConfig() const;

private:
    CacheManager() = default;
    std::map<std::string, std::shared_ptr<void>> factories_;
    CacheConfig global_config_;
};

} // namespace cache
} // namespace mpim
