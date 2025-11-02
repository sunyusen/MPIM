/**
 * 带固定TTL的LRU算法实现
 * 
 * 核心思想：
 * - 在标准LRU基础上，为每个节点添加过期时间
 * - 所有节点使用相同的TTL（生存时间）
 * - Get时检查是否过期，Put时自动设置过期时间
 * - 可选：使用定时器或惰性删除清理过期数据
 * 
 * 时间复杂度：
 * - Get: O(1) + 惰性删除
 * - Put: O(1)
 * 
 * 空间复杂度：O(capacity)
 */

#include <iostream>
#include <unordered_map>
#include <list>
#include <optional>
#include <chrono>

template<typename Key, typename Value>
class LRUCacheWithFixedTTL {
private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::milliseconds;
    
    struct CacheNode {
        Key key;
        Value value;
        TimePoint expire_time;  // 过期时间点
        
        CacheNode(const Key& k, const Value& v, TimePoint exp)
            : key(k), value(v), expire_time(exp) {}
    };
    
    using ListIterator = typename std::list<CacheNode>::iterator;
    
    size_t capacity_;
    Duration ttl_;  // 固定的TTL时间
    std::list<CacheNode> cache_list_;
    std::unordered_map<Key, ListIterator> cache_map_;
    
    // 检查节点是否过期
    bool isExpired(const CacheNode& node) const {
        return Clock::now() >= node.expire_time;
    }
    
    // 移动到链表头部
    void moveToFront(ListIterator it) {
        cache_list_.splice(cache_list_.begin(), cache_list_, it);
    }
    
    // 删除最久未使用的元素
    void evictLRU() {
        if (cache_list_.empty()) return;
        
        auto& lru_node = cache_list_.back();
        cache_map_.erase(lru_node.key);
        cache_list_.pop_back();
    }
    
    // 惰性删除：删除过期节点
    void removeExpiredNode(ListIterator it) {
        cache_map_.erase(it->key);
        cache_list_.erase(it);
    }
    
    // 计算新的过期时间
    TimePoint calculateExpireTime() const {
        return Clock::now() + ttl_;
    }

public:
    // ttl_ms: TTL时间（毫秒）
    explicit LRUCacheWithFixedTTL(size_t capacity, uint64_t ttl_ms)
        : capacity_(capacity), ttl_(ttl_ms) {
        if (capacity == 0) {
            throw std::invalid_argument("Capacity must be greater than 0");
        }
        if (ttl_ms == 0) {
            throw std::invalid_argument("TTL must be greater than 0");
        }
    }
    
    // 获取缓存值（会检查过期）
    std::optional<Value> get(const Key& key) {
        auto it = cache_map_.find(key);
        if (it == cache_map_.end()) {
            return std::nullopt;
        }
        
        // 检查是否过期
        if (isExpired(*(it->second))) {
            removeExpiredNode(it->second);
            return std::nullopt;
        }
        
        // 移到链表头部
        moveToFront(it->second);
        return it->second->value;
    }
    
    // 设置缓存值（自动设置过期时间）
    void put(const Key& key, const Value& value) {
        auto it = cache_map_.find(key);
        TimePoint expire_time = calculateExpireTime();
        
        if (it != cache_map_.end()) {
            // 键已存在，更新值和过期时间
            it->second->value = value;
            it->second->expire_time = expire_time;
            moveToFront(it->second);
        } else {
            // 键不存在，插入新节点
            if (cache_list_.size() >= capacity_) {
                evictLRU();
            }
            
            cache_list_.emplace_front(key, value, expire_time);
            cache_map_[key] = cache_list_.begin();
        }
    }
    
    // 清理所有过期的条目（主动清理）
    size_t cleanupExpired() {
        size_t removed_count = 0;
        auto it = cache_list_.begin();
        
        while (it != cache_list_.end()) {
            if (isExpired(*it)) {
                cache_map_.erase(it->key);
                it = cache_list_.erase(it);
                removed_count++;
            } else {
                ++it;
            }
        }
        
        return removed_count;
    }
    
    // 删除指定键
    bool remove(const Key& key) {
        auto it = cache_map_.find(key);
        if (it == cache_map_.end()) {
            return false;
        }
        
        cache_list_.erase(it->second);
        cache_map_.erase(it);
        return true;
    }
    
    // 清空缓存
    void clear() {
        cache_list_.clear();
        cache_map_.clear();
    }
    
    size_t size() const {
        return cache_list_.size();
    }
    
    size_t capacity() const {
        return capacity_;
    }
    
    uint64_t ttl_ms() const {
        return ttl_.count();
    }
    
    // 打印缓存内容（包括剩余时间）
    void print() const {
        auto now = Clock::now();
        std::cout << "LRU Cache with Fixed TTL (size=" << cache_list_.size() 
                  << ", capacity=" << capacity_ 
                  << ", TTL=" << ttl_.count() << "ms):\n";
        
        for (const auto& node : cache_list_) {
            auto remaining = std::chrono::duration_cast<Duration>(
                node.expire_time - now);
            std::cout << "  [" << node.key << ": " << node.value 
                      << ", TTL剩余: " << remaining.count() << "ms]\n";
        }
    }
};

// 使用示例
int main() {
    std::cout << "=== 带固定TTL的LRU算法示例 ===\n\n";
    
    // 创建容量为3，TTL为2000ms的缓存
    LRUCacheWithFixedTTL<int, std::string> cache(3, 2000);
    
    // 插入数据
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    std::cout << "插入 (1,one), (2,two), (3,three):\n";
    cache.print();
    
    // 等待500ms
    std::cout << "\n等待500ms...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 访问键1（会刷新到链表头部）
    std::cout << "访问键1: " << cache.get(1).value_or("not found") << "\n";
    cache.print();
    
    // 等待1600ms（键2和3会过期，但键1因为被访问过所以还有时间）
    std::cout << "\n等待1600ms...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1600));
    
    std::cout << "尝试访问各个键:\n";
    std::cout << "  键1: " << cache.get(1).value_or("已过期") << "\n";
    std::cout << "  键2: " << cache.get(2).value_or("已过期") << "\n";
    std::cout << "  键3: " << cache.get(3).value_or("已过期") << "\n";
    
    cache.print();
    
    // 主动清理过期条目
    std::cout << "\n主动清理过期条目...\n";
    size_t removed = cache.cleanupExpired();
    std::cout << "清理了 " << removed << " 个过期条目\n";
    cache.print();
    
    // 重新插入数据
    std::cout << "\n重新插入数据:\n";
    cache.put(4, "four");
    cache.put(5, "five");
    cache.print();
    
    return 0;
}

