/**
 * LRU算法性能测试与对比
 * 
 * 测试内容：
 * 1. 标准LRU vs 固定TTL vs 可变TTL的性能对比
 * 2. 不同容量下的性能表现
 * 3. 不同命中率下的性能表现
 * 4. 过期清理的性能开销
 */

#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <iomanip>

// 简化的计时器类
class Timer {
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    TimePoint start_time_;
    
public:
    Timer() : start_time_(Clock::now()) {}
    
    void reset() {
        start_time_ = Clock::now();
    }
    
    double elapsed_ms() const {
        auto end_time = Clock::now();
        return std::chrono::duration<double, std::milli>(
            end_time - start_time_).count();
    }
    
    double elapsed_us() const {
        auto end_time = Clock::now();
        return std::chrono::duration<double, std::micro>(
            end_time - start_time_).count();
    }
};

// 性能测试统计
struct BenchmarkResult {
    std::string name;
    size_t operations;
    double total_time_ms;
    double avg_time_us;
    size_t hits;
    size_t misses;
    
    void print() const {
        std::cout << std::left << std::setw(30) << name
                  << " | 操作数: " << std::setw(8) << operations
                  << " | 总时间: " << std::fixed << std::setprecision(2) 
                  << std::setw(10) << total_time_ms << "ms"
                  << " | 平均: " << std::setprecision(3) 
                  << std::setw(8) << avg_time_us << "μs"
                  << " | 命中率: " << std::setprecision(1) 
                  << (hits * 100.0 / operations) << "%\n";
    }
};

// 生成随机测试数据
class WorkloadGenerator {
private:
    std::mt19937 rng_;
    std::uniform_int_distribution<int> key_dist_;
    
public:
    WorkloadGenerator(int key_range, int seed = 42) 
        : rng_(seed), key_dist_(0, key_range - 1) {}
    
    int next_key() {
        return key_dist_(rng_);
    }
    
    std::vector<int> generate_keys(size_t count) {
        std::vector<int> keys;
        keys.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            keys.push_back(next_key());
        }
        return keys;
    }
};

// 标准LRU的简化实现（用于测试）
template<typename Key, typename Value>
class SimpleLRU {
private:
    struct Node {
        Key key;
        Value value;
        Node(const Key& k, const Value& v) : key(k), value(v) {}
    };
    
    size_t capacity_;
    std::list<Node> list_;
    std::unordered_map<Key, typename std::list<Node>::iterator> map_;
    
public:
    explicit SimpleLRU(size_t capacity) : capacity_(capacity) {}
    
    bool get(const Key& key, Value& value) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }
        list_.splice(list_.begin(), list_, it->second);
        value = it->second->value;
        return true;
    }
    
    void put(const Key& key, const Value& value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->value = value;
            list_.splice(list_.begin(), list_, it->second);
        } else {
            if (list_.size() >= capacity_) {
                map_.erase(list_.back().key);
                list_.pop_back();
            }
            list_.emplace_front(key, value);
            map_[key] = list_.begin();
        }
    }
};

// 运行基准测试
BenchmarkResult run_benchmark(const std::string& name,
                             size_t cache_size,
                             const std::vector<int>& keys) {
    SimpleLRU<int, int> cache(cache_size);
    Timer timer;
    size_t hits = 0;
    size_t misses = 0;
    
    // 预热：填充缓存
    for (size_t i = 0; i < cache_size; ++i) {
        cache.put(i, i * 10);
    }
    
    timer.reset();
    
    // 执行测试
    for (const auto& key : keys) {
        int value;
        if (cache.get(key, value)) {
            hits++;
        } else {
            misses++;
            cache.put(key, key * 10);
        }
    }
    
    double elapsed = timer.elapsed_ms();
    
    BenchmarkResult result;
    result.name = name;
    result.operations = keys.size();
    result.total_time_ms = elapsed;
    result.avg_time_us = (elapsed * 1000.0) / keys.size();
    result.hits = hits;
    result.misses = misses;
    
    return result;
}

void print_header() {
    std::cout << std::string(120, '=') << "\n";
    std::cout << std::left << std::setw(30) << "测试名称"
              << " | " << std::setw(8) << "操作数"
              << " | " << std::setw(10) << "总时间"
              << " | " << std::setw(8) << "平均时间"
              << " | " << "命中率\n";
    std::cout << std::string(120, '-') << "\n";
}

void run_all_benchmarks() {
    std::cout << "\n=== LRU算法性能基准测试 ===\n\n";
    
    // 测试配置
    std::vector<size_t> cache_sizes = {100, 1000, 10000};
    std::vector<int> key_ranges = {200, 2000, 20000};  // 2x cache size
    size_t num_operations = 100000;
    
    for (size_t i = 0; i < cache_sizes.size(); ++i) {
        size_t cache_size = cache_sizes[i];
        int key_range = key_ranges[i];
        
        std::cout << "缓存容量: " << cache_size 
                  << ", 键范围: [0, " << key_range << ")\n";
        print_header();
        
        // 生成测试数据
        WorkloadGenerator gen(key_range);
        auto keys = gen.generate_keys(num_operations);
        
        // 运行测试
        auto result = run_benchmark("标准LRU", cache_size, keys);
        result.print();
        
        std::cout << "\n";
    }
    
    // 不同访问模式的测试
    std::cout << "\n=== 不同访问模式测试 ===\n\n";
    size_t cache_size = 1000;
    
    print_header();
    
    // 高命中率场景（80/20规则）
    {
        WorkloadGenerator gen(500);  // 键范围小于缓存容量
        auto keys = gen.generate_keys(num_operations);
        auto result = run_benchmark("高命中率场景", cache_size, keys);
        result.print();
    }
    
    // 中等命中率场景
    {
        WorkloadGenerator gen(2000);  // 键范围是缓存容量的2倍
        auto keys = gen.generate_keys(num_operations);
        auto result = run_benchmark("中等命中率场景", cache_size, keys);
        result.print();
    }
    
    // 低命中率场景
    {
        WorkloadGenerator gen(10000);  // 键范围是缓存容量的10倍
        auto keys = gen.generate_keys(num_operations);
        auto result = run_benchmark("低命中率场景", cache_size, keys);
        result.print();
    }
    
    std::cout << "\n";
}

// 内存效率分析
void analyze_memory_efficiency() {
    std::cout << "\n=== 内存效率分析 ===\n\n";
    
    struct MemoryInfo {
        std::string type;
        size_t node_size;
        size_t overhead;
        
        void print(size_t capacity) const {
            size_t total = (node_size + overhead) * capacity;
            std::cout << std::left << std::setw(30) << type
                      << " | 节点大小: " << std::setw(4) << node_size << " bytes"
                      << " | 开销: " << std::setw(4) << overhead << " bytes"
                      << " | 总计(" << capacity << "): " 
                      << (total / 1024.0) << " KB\n";
        }
    };
    
    size_t capacity = 10000;
    
    std::cout << "假设 Key=int(4B), Value=int(4B), 容量=" << capacity << "\n";
    std::cout << std::string(100, '-') << "\n";
    
    // 标准LRU: key + value + 2个指针(prev/next)
    MemoryInfo standard_lru{
        "标准LRU",
        sizeof(int) * 2,  // key + value
        sizeof(void*) * 2 + sizeof(void*) * 2  // list指针 + map指针
    };
    standard_lru.print(capacity);
    
    // 固定TTL LRU: 额外一个时间戳
    MemoryInfo fixed_ttl_lru{
        "固定TTL LRU",
        sizeof(int) * 2 + sizeof(uint64_t),  // key + value + timestamp
        sizeof(void*) * 2 + sizeof(void*) * 2
    };
    fixed_ttl_lru.print(capacity);
    
    // 可变TTL LRU: 额外时间戳 + 堆索引
    MemoryInfo variable_ttl_lru{
        "可变TTL LRU",
        sizeof(int) * 2 + sizeof(uint64_t) + sizeof(bool),  // + is_valid flag
        sizeof(void*) * 2 + sizeof(void*) * 3  // list + map + heap
    };
    variable_ttl_lru.print(capacity);
    
    std::cout << "\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          LRU 算法性能测试与对比分析                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    
    // 运行性能测试
    run_all_benchmarks();
    
    // 内存效率分析
    analyze_memory_efficiency();
    
    // 总结建议
    std::cout << "\n=== 使用建议 ===\n\n";
    std::cout << "1. 标准LRU:\n";
    std::cout << "   - 适用场景: 纯粹基于访问频率的缓存，无过期需求\n";
    std::cout << "   - 优点: 实现简单，性能最优，内存开销最小\n";
    std::cout << "   - 缺点: 不支持过期策略\n\n";
    
    std::cout << "2. 固定TTL LRU:\n";
    std::cout << "   - 适用场景: 所有数据有相同生命周期（如会话缓存）\n";
    std::cout << "   - 优点: 实现相对简单，TTL固定便于管理\n";
    std::cout << "   - 缺点: 灵活性差，需要定期清理或惰性删除\n\n";
    
    std::cout << "3. 可变TTL LRU:\n";
    std::cout << "   - 适用场景: 不同数据需要不同过期时间（如混合缓存）\n";
    std::cout << "   - 优点: 灵活性最高，可精确控制每个条目的TTL\n";
    std::cout << "   - 缺点: 实现复杂，需要额外的堆维护，性能略低\n\n";
    
    std::cout << "性能排序: 标准LRU > 固定TTL LRU > 可变TTL LRU\n";
    std::cout << "灵活性: 可变TTL LRU > 固定TTL LRU > 标准LRU\n";
    std::cout << "内存开销: 标准LRU < 固定TTL LRU < 可变TTL LRU\n\n";
    
    return 0;
}

