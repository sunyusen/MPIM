# LRU算法最佳实践与实现

本目录包含三种LRU（Least Recently Used）缓存算法的完整实现和性能对比。

## 📁 文件说明

- `standard_lru.cc` - 标准LRU算法实现
- `lru_with_fixed_ttl.cc` - 带固定TTL的LRU算法
- `lru_with_variable_ttl.cc` - 支持不同TTL的LRU算法（最优方案）
- `benchmark.cc` - 性能测试与对比分析
- `CMakeLists.txt` - 编译配置文件

## 🎯 三种算法对比

### 1. 标准LRU算法

**数据结构:**
- 双向链表：维护访问顺序（最近使用的在头部）
- 哈希表：实现O(1)查找

**时间复杂度:**
- Get: O(1)
- Put: O(1)

**空间复杂度:** O(capacity)

**适用场景:**
- 纯粹基于访问频率的缓存
- 无过期时间需求
- 追求极致性能

**优点:**
- 实现简洁
- 性能最优
- 内存开销最小

**缺点:**
- 不支持过期策略
- 无法自动清理旧数据

---

### 2. 带固定TTL的LRU算法

**数据结构:**
- 标准LRU + 每个节点的过期时间戳
- 所有节点使用相同的TTL

**时间复杂度:**
- Get: O(1) + 惰性删除
- Put: O(1)
- CleanupExpired: O(n)

**空间复杂度:** O(capacity)

**适用场景:**
- 所有数据具有相同生命周期
- 会话缓存（Session Cache）
- 临时数据存储

**优点:**
- 实现相对简单
- TTL统一管理方便
- 自动过期机制

**缺点:**
- 灵活性较差
- 全量清理开销大
- 无法针对不同数据设置不同过期时间

**清理策略:**
1. **惰性删除**: Get时检查是否过期
2. **主动清理**: 定期调用`cleanupExpired()`

---

### 3. 支持不同TTL的LRU算法 ⭐（推荐）

**数据结构:**
- 双向链表：维护LRU顺序
- 哈希表：快速查找
- 最小堆（优先队列）：按过期时间排序
- 有效性标记：避免堆中重复删除

**时间复杂度:**
- Get: O(1) + 惰性删除
- Put: O(log n) （堆插入）
- CleanupExpired: O(k log n)，k为过期数量

**空间复杂度:** O(capacity)

**适用场景:**
- 不同数据需要不同过期时间
- 混合缓存（配置、会话、临时数据）
- 需要精确控制每个条目生命周期

**优点:**
- 灵活性最高
- 可精确控制每个条目TTL
- 高效的过期清理（堆顶是最早过期的）
- 支持获取下一个过期时间

**缺点:**
- 实现复杂度最高
- 需要维护额外的堆结构
- Put操作略慢（O(log n)）

**核心优化:**
1. **最小堆加速**: 快速定位最早过期的元素
2. **有效性标记**: 避免堆中删除操作（O(log n)）
3. **惰性删除 + 定期清理**: 平衡性能和内存

---

## 📊 性能对比

| 算法类型 | Get操作 | Put操作 | 清理操作 | 内存开销 | 灵活性 |
|---------|---------|---------|----------|---------|--------|
| 标准LRU | O(1) | O(1) | N/A | 最小 | 低 |
| 固定TTL | O(1) | O(1) | O(n) | 中等 | 中 |
| 可变TTL | O(1) | O(log n) | O(k log n) | 较大 | 高 |

**内存对比** (Key=4B, Value=4B, 容量=10000):
- 标准LRU: ~240 KB
- 固定TTL LRU: ~320 KB
- 可变TTL LRU: ~400 KB

---

## 🚀 编译运行

### 使用CMake编译

```bash
cd examples/lru_algorithms
mkdir build && cd build
cmake ..
make

# 运行示例
./standard_lru
./lru_fixed_ttl
./lru_variable_ttl

# 运行性能测试
./benchmark
```

### 手动编译

```bash
# 标准LRU
g++ -std=c++17 -O2 standard_lru.cc -o standard_lru

# 固定TTL
g++ -std=c++17 -O2 lru_with_fixed_ttl.cc -o lru_fixed_ttl -lpthread

# 可变TTL
g++ -std=c++17 -O2 lru_with_variable_ttl.cc -o lru_variable_ttl -lpthread

# 性能测试
g++ -std=c++17 -O2 benchmark.cc -o benchmark
```

---

## 💡 使用示例

### 标准LRU

```cpp
#include "standard_lru.cc"

LRUCache<int, std::string> cache(100);  // 容量100
cache.put(1, "value1");
auto value = cache.get(1);  // 返回 optional<string>
```

### 固定TTL LRU

```cpp
#include "lru_with_fixed_ttl.cc"

// 容量100，TTL=5000ms
LRUCacheWithFixedTTL<int, std::string> cache(100, 5000);
cache.put(1, "value1");  // 5秒后过期

// 主动清理过期数据
size_t removed = cache.cleanupExpired();
```

### 可变TTL LRU

```cpp
#include "lru_with_variable_ttl.cc"

LRUCacheWithVariableTTL<string, string> cache(100);

// 不同数据设置不同TTL
cache.put("session", "data1", 1000);   // 1秒
cache.put("config", "data2", 60000);   // 60秒
cache.put("cache", "data3", 300000);   // 5分钟

// 获取下一个过期时间
auto next_expire = cache.getNextExpireTime();

// 清理过期数据
size_t removed = cache.cleanupExpired();
```

---

## 🎓 核心实现要点

### 1. 标准LRU关键设计

```cpp
// 核心数据结构
std::list<CacheNode> cache_list_;              // 双向链表
std::unordered_map<Key, ListIterator> cache_map_;  // 哈希表

// 关键操作
void moveToFront(ListIterator it) {
    cache_list_.splice(cache_list_.begin(), cache_list_, it);
}
```

### 2. 固定TTL关键设计

```cpp
struct CacheNode {
    Key key;
    Value value;
    TimePoint expire_time;  // 过期时间戳
};

// Get时检查过期
if (Clock::now() >= node.expire_time) {
    removeExpiredNode(it);
    return std::nullopt;
}
```

### 3. 可变TTL关键设计

```cpp
// 最小堆：按过期时间排序
std::priority_queue<ListIterator, 
                   vector<ListIterator>, 
                   ExpireComparator> expire_queue_;

// 有效性标记避免堆中删除
struct CacheNode {
    // ... 其他字段
    bool is_valid;  // 标记节点是否有效
};

// 高效清理：从堆顶开始
size_t cleanupExpired() {
    while (!expire_queue_.empty()) {
        auto it = expire_queue_.top();
        if (!it->is_valid) {
            expire_queue_.pop();
            continue;
        }
        if (it->expire_time > now) break;
        removeNode(it);
        expire_queue_.pop();
    }
}
```

---

## 🔧 高级优化技巧

### 1. 线程安全版本

添加读写锁：

```cpp
#include <shared_mutex>

class ThreadSafeLRU {
    mutable std::shared_mutex mutex_;
    
    std::optional<Value> get(const Key& key) {
        std::shared_lock lock(mutex_);  // 读锁
        // ... 实现
    }
    
    void put(const Key& key, const Value& value) {
        std::unique_lock lock(mutex_);  // 写锁
        // ... 实现
    }
};
```

### 2. 分段锁优化

```cpp
// 将缓存分成多个段，降低锁竞争
template<typename Key, typename Value>
class SegmentedLRU {
    static constexpr size_t NUM_SEGMENTS = 16;
    std::array<LRUCache<Key, Value>, NUM_SEGMENTS> segments_;
    
    size_t getSegment(const Key& key) {
        return std::hash<Key>{}(key) % NUM_SEGMENTS;
    }
};
```

### 3. 定时清理

```cpp
// 使用定时器定期清理过期数据
class AutoCleanupLRU : public LRUCacheWithVariableTTL {
    std::thread cleanup_thread_;
    std::atomic<bool> running_{true};
    
    void cleanupLoop() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            cleanupExpired();
        }
    }
};
```

---

## 📈 性能测试结果

运行 `./benchmark` 查看详细性能报告，包括：

1. **不同容量下的性能对比**
2. **不同命中率场景测试**
3. **内存效率分析**
4. **使用建议**

---

## 🎯 选择建议

| 场景 | 推荐算法 | 理由 |
|------|---------|------|
| 数据库查询缓存 | 标准LRU | 无过期需求，追求性能 |
| HTTP会话存储 | 固定TTL | 统一30分钟过期 |
| Web应用混合缓存 | 可变TTL | 静态资源1天，API结果5分钟 |
| Redis类缓存系统 | 可变TTL | 支持per-key TTL |
| 页面缓存 | 固定TTL | 统一刷新策略 |

---

## 📚 参考资料

1. **LeetCode 146**: LRU Cache 经典题
2. **Redis源码**: eviction策略实现
3. **Caffeine**: Google Guava缓存库的继任者
4. **论文**: "The LRU-K Page Replacement Algorithm"

---

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📄 License

MIT License

