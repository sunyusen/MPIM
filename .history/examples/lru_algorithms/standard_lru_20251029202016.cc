/**
 * 标准LRU算法实现
 * 
 * 核心思想：
 * - 使用双向链表维护访问顺序（最近使用的在链表头部）
 * - 使用哈希表实现O(1)时间复杂度的查找
 * 
 * 时间复杂度：
 * - Get: O(1)
 * - Put: O(1)
 * 
 * 空间复杂度：O(capacity)
 */

#include <iostream>
#include <unordered_map>
#include <list>
#include <optional>

template<typename Key, typename Value>
class LRUCache {
private:
    // 双向链表存储键值对，最近使用的在前面
    struct CacheNode {
        Key key;
        Value value;
        CacheNode(const Key& k, const Value& v) : key(k), value(v) {}
    };
    
    using ListIterator = typename std::list<CacheNode>::iterator;
    
    size_t capacity_;
    std::list<CacheNode> cache_list_;  // 双向链表
    std::unordered_map<Key, ListIterator> cache_map_;  // 哈希表：key -> 链表迭代器
    
    // 将节点移动到链表头部（表示最近使用）
    void moveToFront(ListIterator it) {
        cache_list_.splice(cache_list_.begin(), cache_list_, it);
    }
    
    // 删除最久未使用的元素（链表尾部）
    void evictLRU() {
        if (cache_list_.empty()) return;
        
        auto& lru_node = cache_list_.back();
        cache_map_.erase(lru_node.key);
        cache_list_.pop_back();
    }

public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("Capacity must be greater than 0");
        }
    }
    
    // 获取缓存值
    std::optional<Value> get(const Key& key) {
        auto it = cache_map_.find(key);
        if (it == cache_map_.end()) {
            return std::nullopt;  // 缓存未命中
        }
        
        // 将访问的节点移到链表头部
        moveToFront(it->second);
        return it->second->value;
    }
    
    // 设置缓存值
    void put(const Key& key, const Value& value) {
        auto it = cache_map_.find(key);
        
        if (it != cache_map_.end()) {
            // 键已存在，更新值并移到链表头部
            it->second->value = value;
            moveToFront(it->second);
        } else {
            // 键不存在，插入新节点
            if (cache_list_.size() >= capacity_) {
                evictLRU();  // 先淘汰最久未使用的元素
            }
            
            cache_list_.emplace_front(key, value);
            cache_map_[key] = cache_list_.begin();
        }
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
    
    // 获取当前缓存大小
    size_t size() const {
        return cache_list_.size();
    }
    
    // 获取容量
    size_t capacity() const {
        return capacity_;
    }
    
    // 是否为空
    bool empty() const {
        return cache_list_.empty();
    }
    
    // 打印缓存内容（从最近到最久）
    void print() const {
        std::cout << "LRU Cache (size=" << cache_list_.size() 
                  << ", capacity=" << capacity_ << "):\n";
        for (const auto& node : cache_list_) {
            std::cout << "  [" << node.key << ": " << node.value << "]\n";
        }
    }
};

// 使用示例
int main() {
    std::cout << "=== 标准LRU算法示例 ===\n\n";
    
    LRUCache<int, std::string> cache(3);
    
    // 插入数据
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    std::cout << "插入 (1,one), (2,two), (3,three):\n";
    cache.print();
    
    // 访问键1
    std::cout << "\n访问键1: " << cache.get(1).value_or("not found") << "\n";
    cache.print();
    
    // 插入新键4（会淘汰最久未使用的键2）
    std::cout << "\n插入 (4,four) - 容量已满，淘汰最久未使用:\n";
    cache.put(4, "four");
    cache.print();
    
    // 验证键2被淘汰
    std::cout << "\n查找键2: " << cache.get(2).value_or("not found") << "\n";
    
    // 更新键3
    std::cout << "\n更新键3为 'THREE':\n";
    cache.put(3, "THREE");
    cache.print();
    
    return 0;
}

