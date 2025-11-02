/**
 * 支持不同TTL的LRU算法实现（最优方案）
 * 
 * 核心思想：
 * - 在标准LRU基础上，每个节点可以有不同的TTL
 * - 使用最小堆（优先队列）维护过期时间，实现高效的过期检查
 * - 双重索引：LRU链表（访问顺序） + 过期时间堆（过期顺序）
 * - 支持主动清理和惰性删除两种策略
 * 
 * 时间复杂度：
 * - Get: O(1) + 惰性删除
 * - Put: O(log n) （堆操作）
 * - CleanupExpired: O(k log n)，k为过期数量
 * 
 * 空间复杂度：O(capacity)
 * 
 * 优化点：
 * 1. 使用最小堆快速定位过期元素
 * 2. 惰性删除 + 定期清理结合
 * 3. 过期节点标记，避免堆中重复
 */

#include <iostream>
#include <unordered_map>
#include <list>
#include <queue>
#include <optional>
#include <chrono>
#include <memory>
#include <functional>

template<typename Key, typename Value>
class LRUCacheWithVariableTTL {
private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::milliseconds;
    
    struct CacheNode {
        Key key;
        Value value;
        TimePoint expire_time;
        bool is_valid;  // 标记节点是否有效（用于处理堆中的过期引用）
        
        CacheNode(const Key& k, const Value& v, TimePoint exp)
            : key(k), value(v), expire_time(exp), is_valid(true) {}
    };
    
    using ListIterator = typename std::list<CacheNode>::iterator;
    
    // 过期时间堆的比较器（最早过期的在堆顶）
    struct ExpireComparator {
        bool operator()(const ListIterator& a, const ListIterator& b) const {
            return a->expire_time > b->expire_time;
        }
    };
    
    size_t capacity_;
    std::list<CacheNode> cache_list_;  // LRU链表
    std::unordered_map<Key, ListIterator> cache_map_;  // 快速查找
    std::priority_queue<ListIterator, 
                       std::vector<ListIterator>, 
                       ExpireComparator> expire_queue_;  // 过期时间最小堆
    
    bool isExpired(const CacheNode& node) const {
        return Clock::now() >= node.expire_time;
    }
    
    void moveToFront(ListIterator it) {
        cache_list_.splice(cache_list_.begin(), cache_list_, it);
    }
    
    void evictLRU() {
        if (cache_list_.empty()) return;
        
        auto& lru_node = cache_list_.back();
        lru_node.is_valid = false;  // 标记为无效
        cache_map_.erase(lru_node.key);
        cache_list_.pop_back();
    }
    
    void removeNode(ListIterator it) {
        it->is_valid = false;  // 标记为无效
        cache_map_.erase(it->key);
        cache_list_.erase(it);
    }
    
    TimePoint calculateExpireTime(uint64_t ttl_ms) const {
        return Clock::now() + Duration(ttl_ms);
    }

public:
    explicit LRUCacheWithVariableTTL(size_t capacity) : capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("Capacity must be greater than 0");
        }
    }
    
    // 获取缓存值
    std::optional<Value> get(const Key& key) {
        auto it = cache_map_.find(key);
        if (it == cache_map_.end()) {
            return std::nullopt;
        }
        
        // 检查是否过期
        if (!it->second->is_valid || isExpired(*(it->second))) {
            if (it->second->is_valid) {
                removeNode(it->second);
            }
            return std::nullopt;
        }
        
        // 移到链表头部（LRU更新）
        moveToFront(it->second);
        return it->second->value;
    }
    
    // 设置缓存值（可指定不同的TTL）
    void put(const Key& key, const Value& value, uint64_t ttl_ms) {
        if (ttl_ms == 0) {
            throw std::invalid_argument("TTL must be greater than 0");
        }
        
        auto it = cache_map_.find(key);
        TimePoint expire_time = calculateExpireTime(ttl_ms);
        
        if (it != cache_map_.end()) {
            // 键已存在，更新值和过期时间
            it->second->value = value;
            it->second->expire_time = expire_time;
            it->second->is_valid = true;
            moveToFront(it->second);
            
            // 将新的过期时间加入堆（旧的引用会被标记为无效）
            expire_queue_.push(it->second);
        } else {
            // 键不存在，插入新节点
            if (cache_list_.size() >= capacity_) {
                evictLRU();
            }
            
            cache_list_.emplace_front(key, value, expire_time);
            auto new_it = cache_list_.begin();
            cache_map_[key] = new_it;
            expire_queue_.push(new_it);
        }
    }
    
    // 使用默认TTL的put（1000ms）
    void put(const Key& key, const Value& value) {
        put(key, value, 1000);
    }
    
    // 清理所有过期的条目（主动清理）
    // 从堆顶开始清理，直到遇到未过期的元素
    size_t cleanupExpired() {
        size_t removed_count = 0;
        auto now = Clock::now();
        
        while (!expire_queue_.empty()) {
            auto it = expire_queue_.top();
            
            // 如果节点已经被标记为无效，直接弹出堆
            if (!it->is_valid) {
                expire_queue_.pop();
                continue;
            }
            
            // 如果未过期，停止清理（堆顶是最早过期的）
            if (it->expire_time > now) {
                break;
            }
            
            // 过期了，删除节点
            removeNode(it);
            expire_queue_.pop();
            removed_count++;
        }
        
        return removed_count;
    }
    
    // 删除指定键
    bool remove(const Key& key) {
        auto it = cache_map_.find(key);
        if (it == cache_map_.end()) {
            return false;
        }
        
        removeNode(it->second);
        return true;
    }
    
    // 清空缓存
    void clear() {
        cache_list_.clear();
        cache_map_.clear();
        while (!expire_queue_.empty()) {
            expire_queue_.pop();
        }
    }
    
    size_t size() const {
        return cache_map_.size();
    }
    
    size_t capacity() const {
        return capacity_;
    }
    
    // 获取下一个过期的时间点（用于定时清理）
    std::optional<TimePoint> getNextExpireTime() const {
        if (expire_queue_.empty()) {
            return std::nullopt;
        }
        
        // 跳过无效节点找到第一个有效节点
        auto temp_queue = expire_queue_;
        while (!temp_queue.empty()) {
            auto it = temp_queue.top();
            if (it->is_valid) {
                return it->expire_time;
            }
            temp_queue.pop();
        }
        
        return std::nullopt;
    }
    
    // 打印缓存内容
    void print() const {
        auto now = Clock::now();
        std::cout << "LRU Cache with Variable TTL (size=" << cache_map_.size() 
                  << ", capacity=" << capacity_ << "):\n";
        
        for (const auto& node : cache_list_) {
            if (!node.is_valid) continue;
            
            auto remaining = std::chrono::duration_cast<Duration>(
                node.expire_time - now);
            std::cout << "  [" << node.key << ": " << node.value 
                      << ", TTL剩余: " << remaining.count() << "ms]\n";
        }
    }
    
    // 打印统计信息
    void printStats() const {
        std::cout << "缓存统计:\n";
        std::cout << "  当前大小: " << cache_map_.size() << "\n";
        std::cout << "  容量: " << capacity_ << "\n";
        std::cout << "  使用率: " << (cache_map_.size() * 100.0 / capacity_) << "%\n";
        
        auto next_expire = getNextExpireTime();
        if (next_expire.has_value()) {
            auto remaining = std::chrono::duration_cast<Duration>(
                next_expire.value() - Clock::now());
            std::cout << "  下一个过期时间: " << remaining.count() << "ms\n";
        } else {
            std::cout << "  下一个过期时间: 无\n";
        }
    }
};

// 使用示例
int main() {
    std::cout << "=== 支持不同TTL的LRU算法示例 ===\n\n";
    
    LRUCacheWithVariableTTL<std::string, std::string> cache(5);
    
    // 插入不同TTL的数据
    std::cout << "插入不同TTL的数据:\n";
    cache.put("session_short", "短期会话", 1000);   // 1秒
    cache.put("session_medium", "中期会话", 3000);  // 3秒
    cache.put("session_long", "长期会话", 5000);    // 5秒
    cache.put("config", "配置数据", 10000);         // 10秒
    cache.print();
    
    std::cout << "\n";
    cache.printStats();
    
    // 等待1.5秒
    std::cout << "\n等待1.5秒...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // 主动清理过期数据
    std::cout << "主动清理过期数据:\n";
    size_t removed = cache.cleanupExpired();
    std::cout << "清理了 " << removed << " 个过期条目\n";
    cache.print();
    
    // 访问中期会话（会移到LRU链表头部）
    std::cout << "\n访问 'session_medium': " 
              << cache.get("session_medium").value_or("已过期") << "\n";
    
    // 等待2秒
    std::cout << "\n等待2秒...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // 尝试访问各个键
    std::cout << "尝试访问各个键:\n";
    std::cout << "  session_short: " << cache.get("session_short").value_or("已过期") << "\n";
    std::cout << "  session_medium: " << cache.get("session_medium").value_or("已过期") << "\n";
    std::cout << "  session_long: " << cache.get("session_long").value_or("已过期") << "\n";
    std::cout << "  config: " << cache.get("config").value_or("已过期") << "\n";
    
    cache.print();
    
    // 测试容量限制
    std::cout << "\n测试容量限制（插入6个元素，容量为5）:\n";
    cache.put("user1", "用户1", 8000);
    cache.put("user2", "用户2", 8000);
    cache.put("user3", "用户3", 8000);
    cache.put("user4", "用户4", 8000);
    cache.put("user5", "用户5", 8000);
    cache.put("user6", "用户6", 8000);  // 这个会触发LRU淘汰
    
    cache.print();
    cache.printStats();
    
    return 0;
}

