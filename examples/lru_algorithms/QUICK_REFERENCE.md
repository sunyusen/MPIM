# LRU算法快速参考手册

## 🚀 快速开始

### 10秒速览

```cpp
// 标准LRU - 最简单最快
LRUCache<int, string> cache(100);
cache.put(1, "value");
auto val = cache.get(1);  // optional<string>

// 固定TTL - 所有数据同一过期时间
LRUCacheWithFixedTTL<int, string> cache(100, 5000);  // 5秒过期
cache.put(1, "value");

// 可变TTL - 每个数据不同过期时间
LRUCacheWithVariableTTL<int, string> cache(100);
cache.put(1, "value", 1000);   // 1秒
cache.put(2, "value", 60000);  // 60秒
```

---

## 📊 三种算法对比表

| 特性 | 标准LRU | 固定TTL | 可变TTL |
|-----|---------|---------|---------|
| **Get速度** | ⚡️⚡️⚡️ 最快 | ⚡️⚡️ 快 | ⚡️⚡️ 快 |
| **Put速度** | ⚡️⚡️⚡️ O(1) | ⚡️⚡️ O(1) | ⚡️ O(log n) |
| **内存开销** | 💚 最小 | 💛 中等 | 💛 较大 |
| **过期支持** | ❌ 无 | ✅ 统一 | ✅ 独立 |
| **灵活性** | ⭐️ 低 | ⭐️⭐️ 中 | ⭐️⭐️⭐️ 高 |
| **实现复杂度** | 🟢 简单 | 🟡 中等 | 🔴 复杂 |

---

## 🎯 选择决策树

```
需要过期功能？
├─ 否 → 标准LRU
│      └─ 场景：数据库查询缓存、计算结果缓存
│
└─ 是 → 所有数据过期时间相同？
       ├─ 是 → 固定TTL LRU
       │      └─ 场景：会话存储、临时文件缓存
       │
       └─ 否 → 可变TTL LRU
              └─ 场景：混合缓存、多租户系统、CDN
```

---

## 📝 核心API对比

### 标准LRU

```cpp
LRUCache<Key, Value> cache(capacity);

// 基本操作
optional<Value> get(const Key& key);
void put(const Key& key, const Value& value);
bool remove(const Key& key);
void clear();

// 查询
size_t size();
size_t capacity();
bool empty();
```

### 固定TTL LRU

```cpp
LRUCacheWithFixedTTL<Key, Value> cache(capacity, ttl_ms);

// 基本操作（同标准LRU）
optional<Value> get(const Key& key);      // 自动检查过期
void put(const Key& key, const Value& value);  // 自动设置过期时间

// 过期管理
size_t cleanupExpired();  // 主动清理过期条目
uint64_t ttl_ms();        // 获取TTL
```

### 可变TTL LRU

```cpp
LRUCacheWithVariableTTL<Key, Value> cache(capacity);

// 基本操作
optional<Value> get(const Key& key);
void put(const Key& key, const Value& value, uint64_t ttl_ms);  // 指定TTL

// 高级功能
size_t cleanupExpired();                    // 清理过期条目
optional<TimePoint> getNextExpireTime();    // 获取下次过期时间
void printStats();                          // 打印统计信息
```

---

## 💡 最佳实践

### 1. 容量设置

```cpp
// ❌ 错误：容量太小
LRUCache<int, string> cache(10);  // 频繁淘汰

// ✅ 正确：根据实际需求设置
size_t max_memory_mb = 100;
size_t item_size_kb = 10;
size_t capacity = (max_memory_mb * 1024) / item_size_kb;
LRUCache<int, string> cache(capacity);
```

### 2. TTL设置

```cpp
// ❌ 错误：TTL太短
cache.put(key, value, 100);  // 100ms - 太短

// ✅ 正确：根据数据特性设置
cache.put("session", data, 30 * 60 * 1000);      // 会话：30分钟
cache.put("static_asset", data, 24 * 3600 * 1000); // 静态资源：1天
cache.put("api_result", data, 5 * 60 * 1000);    // API结果：5分钟
```

### 3. 过期清理策略

```cpp
// 方案A：惰性删除（推荐用于低频访问）
// 只在get时检查，无需主动清理

// 方案B：定时清理（推荐用于高频场景）
void cleanup_thread() {
    while (running) {
        sleep(60);  // 每分钟清理一次
        cache.cleanupExpired();
    }
}

// 方案C：混合策略（最优）
// get时惰性删除 + 定期主动清理
auto next_expire = cache.getNextExpireTime();
if (next_expire.has_value()) {
    sleep_until(next_expire.value());
    cache.cleanupExpired();
}
```

### 4. 线程安全

```cpp
// 多线程环境需要加锁
class ThreadSafeLRU {
    mutable shared_mutex mutex_;
    LRUCache<K, V> cache_;
    
public:
    optional<V> get(const K& key) {
        shared_lock lock(mutex_);  // 读锁
        return cache_.get(key);
    }
    
    void put(const K& key, const V& value) {
        unique_lock lock(mutex_);  // 写锁
        cache_.put(key, value);
    }
};
```

---

## ⚠️ 常见陷阱

### 陷阱1：忘记检查optional

```cpp
// ❌ 错误：直接使用可能未初始化的值
auto value = cache.get(key).value();  // 可能抛异常

// ✅ 正确：检查是否存在
if (auto value = cache.get(key); value.has_value()) {
    use(value.value());
} else {
    handle_miss();
}

// ✅ 或使用value_or
auto value = cache.get(key).value_or(default_value);
```

### 陷阱2：容量为0

```cpp
// ❌ 错误
LRUCache<int, int> cache(0);  // 抛出异常

// ✅ 正确
if (capacity > 0) {
    LRUCache<int, int> cache(capacity);
}
```

### 陷阱3：TTL为0

```cpp
// ❌ 错误
cache.put(key, value, 0);  // 立即过期，无意义

// ✅ 正确：设置合理的TTL
cache.put(key, value, 1000);  // 至少1秒
```

### 陷阱4：过度清理

```cpp
// ❌ 错误：清理太频繁
while (true) {
    cache.cleanupExpired();  // CPU占用高
    sleep_ms(10);
}

// ✅ 正确：合理间隔
while (true) {
    cache.cleanupExpired();
    sleep_ms(60000);  // 每分钟清理一次
}
```

---

## 📈 性能优化技巧

### 1. 预分配容量

```cpp
// 预留足够容量，减少rehash
size_t expected_size = 10000;
cache_map_.reserve(expected_size);
```

### 2. 批量操作

```cpp
// ❌ 低效：逐个插入
for (auto& item : items) {
    cache.put(item.key, item.value);
}

// ✅ 高效：批量插入（如果实现支持）
cache.putBatch(items);
```

### 3. 分段锁

```cpp
// 将大缓存分成多个小缓存，降低锁竞争
class ShardedLRU {
    static constexpr size_t SHARDS = 16;
    array<LRUCache<K,V>, SHARDS> caches_;
    
    size_t getShard(const K& key) {
        return hash<K>{}(key) % SHARDS;
    }
};
```

---

## 🔍 调试技巧

### 1. 启用统计

```cpp
class LRUWithStats : public LRUCache<K, V> {
    atomic<size_t> hits_{0};
    atomic<size_t> misses_{0};
    
    optional<V> get(const K& key) override {
        auto result = LRUCache<K,V>::get(key);
        if (result.has_value()) hits_++;
        else misses_++;
        return result;
    }
    
    void printStats() {
        cout << "Hit rate: " 
             << (hits_ * 100.0 / (hits_ + misses_)) << "%\n";
    }
};
```

### 2. 日志关键操作

```cpp
void put(const K& key, const V& value) {
    if (DEBUG) {
        cout << "PUT: " << key << " (size=" << size() << ")\n";
    }
    // ... 实现
}
```

---

## 📚 实际应用场景速查

| 应用场景 | 推荐算法 | 容量建议 | TTL建议 |
|---------|---------|---------|---------|
| Web会话 | 固定TTL | 10K-100K | 30分钟 |
| 数据库查询 | 标准LRU | 1K-10K | N/A |
| CDN缓存 | 可变TTL | 100K-1M | 1小时-1天 |
| API限流 | 标准LRU | 10K | N/A |
| 配置中心 | 可变TTL | 1K | 5分钟-1小时 |
| 图片缩略图 | 固定TTL | 10K-100K | 24小时 |
| DNS缓存 | 可变TTL | 10K | 根据DNS记录 |

---

## 🎓 延伸学习

### 进阶算法

1. **LRU-K**: 考虑K次历史访问
2. **2Q**: 两个队列，区分频率和新近度
3. **ARC**: 自适应替换缓存
4. **LIRS**: Low Inter-reference Recency Set
5. **TinyLFU**: 基于频率的淘汰策略

### 相关技术

- Redis的eviction策略
- Memcached的LRU实现
- Caffeine缓存库
- Linux页面置换算法

---

## 📞 快速诊断

**Q: 缓存命中率低？**
- 增加容量
- 检查访问模式是否适合LRU
- 考虑使用LFU或混合策略

**Q: 内存占用过高？**
- 减小容量
- 缩短TTL
- 增加清理频率

**Q: 性能不佳？**
- 使用分段锁
- 减少过期检查频率
- 考虑使用标准LRU

**Q: 数据一直被淘汰？**
- 容量太小
- 访问模式不适合LRU
- 考虑增加容量或使用优先级队列

---

*最后更新: 2025-10*

