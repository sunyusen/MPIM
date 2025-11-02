/**
 * LRU算法综合使用示例
 * 
 * 展示三种LRU算法在实际场景中的应用
 */

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

// ============================================================================
// 场景1: Web应用的页面缓存（使用固定TTL LRU）
// ============================================================================

class WebPageCache {
private:
    // 简化的固定TTL LRU实现
    struct PageNode {
        std::string url;
        std::string content;
        std::chrono::steady_clock::time_point expire_time;
        PageNode(const std::string& u, const std::string& c, 
                std::chrono::steady_clock::time_point exp)
            : url(u), content(c), expire_time(exp) {}
    };
    
    size_t capacity_;
    std::chrono::milliseconds ttl_;
    std::list<PageNode> pages_;
    std::unordered_map<std::string, std::list<PageNode>::iterator> index_;
    
public:
    WebPageCache(size_t capacity, uint64_t ttl_ms) 
        : capacity_(capacity), ttl_(ttl_ms) {}
    
    std::optional<std::string> getPage(const std::string& url) {
        auto it = index_.find(url);
        if (it == index_.end()) {
            return std::nullopt;
        }
        
        // 检查过期
        if (std::chrono::steady_clock::now() >= it->second->expire_time) {
            pages_.erase(it->second);
            index_.erase(it);
            return std::nullopt;
        }
        
        // 移到前面
        pages_.splice(pages_.begin(), pages_, it->second);
        return it->second->content;
    }
    
    void cachePage(const std::string& url, const std::string& content) {
        auto expire_time = std::chrono::steady_clock::now() + ttl_;
        
        auto it = index_.find(url);
        if (it != index_.end()) {
            it->second->content = content;
            it->second->expire_time = expire_time;
            pages_.splice(pages_.begin(), pages_, it->second);
        } else {
            if (pages_.size() >= capacity_) {
                index_.erase(pages_.back().url);
                pages_.pop_back();
            }
            pages_.emplace_front(url, content, expire_time);
            index_[url] = pages_.begin();
        }
    }
    
    void printStats() const {
        std::cout << "Web页面缓存统计:\n";
        std::cout << "  缓存页面数: " << pages_.size() << "\n";
        std::cout << "  容量: " << capacity_ << "\n";
        std::cout << "  TTL: " << ttl_.count() << "ms\n";
    }
};

void demo_web_page_cache() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "场景1: Web应用的页面缓存（固定TTL LRU）\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    // 页面缓存，容量10，TTL 3秒
    WebPageCache cache(10, 3000);
    
    // 模拟页面访问
    std::cout << "缓存页面...\n";
    cache.cachePage("/home", "<html>首页</html>");
    cache.cachePage("/about", "<html>关于</html>");
    cache.cachePage("/contact", "<html>联系</html>");
    cache.printStats();
    
    // 访问页面
    std::cout << "\n访问 /home: " 
              << cache.getPage("/home").value_or("未缓存") << "\n";
    
    // 等待过期
    std::cout << "\n等待3.5秒，页面过期...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(3500));
    
    std::cout << "再次访问 /home: " 
              << cache.getPage("/home").value_or("未缓存（已过期）") << "\n";
    
    cache.printStats();
}

// ============================================================================
// 场景2: 数据库查询结果缓存（使用标准LRU）
// ============================================================================

class DatabaseQueryCache {
private:
    struct QueryResult {
        std::string sql;
        std::vector<std::string> rows;
        QueryResult(const std::string& s, const std::vector<std::string>& r)
            : sql(s), rows(r) {}
    };
    
    size_t capacity_;
    std::list<QueryResult> results_;
    std::unordered_map<std::string, std::list<QueryResult>::iterator> index_;
    
    size_t hit_count_ = 0;
    size_t miss_count_ = 0;
    
public:
    explicit DatabaseQueryCache(size_t capacity) : capacity_(capacity) {}
    
    std::optional<std::vector<std::string>> query(const std::string& sql) {
        auto it = index_.find(sql);
        if (it != index_.end()) {
            hit_count_++;
            results_.splice(results_.begin(), results_, it->second);
            return it->second->rows;
        }
        
        miss_count_++;
        return std::nullopt;
    }
    
    void cacheResult(const std::string& sql, 
                    const std::vector<std::string>& rows) {
        auto it = index_.find(sql);
        if (it != index_.end()) {
            it->second->rows = rows;
            results_.splice(results_.begin(), results_, it->second);
        } else {
            if (results_.size() >= capacity_) {
                index_.erase(results_.back().sql);
                results_.pop_back();
            }
            results_.emplace_front(sql, rows);
            index_[sql] = results_.begin();
        }
    }
    
    void printStats() const {
        std::cout << "数据库查询缓存统计:\n";
        std::cout << "  缓存条目: " << results_.size() << "\n";
        std::cout << "  容量: " << capacity_ << "\n";
        std::cout << "  命中次数: " << hit_count_ << "\n";
        std::cout << "  未命中次数: " << miss_count_ << "\n";
        if (hit_count_ + miss_count_ > 0) {
            std::cout << "  命中率: " << std::fixed << std::setprecision(1)
                      << (hit_count_ * 100.0 / (hit_count_ + miss_count_)) 
                      << "%\n";
        }
    }
};

void demo_database_cache() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "场景2: 数据库查询结果缓存（标准LRU）\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    DatabaseQueryCache cache(5);
    
    // 模拟数据库查询
    auto executeQuery = [&](const std::string& sql) {
        auto result = cache.query(sql);
        if (result.has_value()) {
            std::cout << "✓ 缓存命中: " << sql << "\n";
            return result.value();
        } else {
            std::cout << "✗ 缓存未命中，执行查询: " << sql << "\n";
            // 模拟数据库查询
            std::vector<std::string> rows = {"row1", "row2", "row3"};
            cache.cacheResult(sql, rows);
            return rows;
        }
    };
    
    // 执行一些查询
    std::cout << "执行查询:\n";
    executeQuery("SELECT * FROM users WHERE id=1");
    executeQuery("SELECT * FROM users WHERE id=2");
    executeQuery("SELECT * FROM users WHERE id=1");  // 命中
    executeQuery("SELECT * FROM products");
    executeQuery("SELECT * FROM users WHERE id=1");  // 命中
    executeQuery("SELECT * FROM orders");
    
    std::cout << "\n";
    cache.printStats();
}

// ============================================================================
// 场景3: 多租户缓存系统（使用可变TTL LRU）
// ============================================================================

class MultiTenantCache {
private:
    struct CacheEntry {
        std::string key;
        std::string value;
        std::string tenant;  // 租户ID
        std::chrono::steady_clock::time_point expire_time;
        bool is_valid;
        
        CacheEntry(const std::string& k, const std::string& v, 
                  const std::string& t, 
                  std::chrono::steady_clock::time_point exp)
            : key(k), value(v), tenant(t), expire_time(exp), is_valid(true) {}
    };
    
    size_t capacity_;
    std::list<CacheEntry> entries_;
    std::unordered_map<std::string, std::list<CacheEntry>::iterator> index_;
    
    std::string makeKey(const std::string& tenant, const std::string& key) {
        return tenant + ":" + key;
    }
    
public:
    explicit MultiTenantCache(size_t capacity) : capacity_(capacity) {}
    
    std::optional<std::string> get(const std::string& tenant, 
                                   const std::string& key) {
        auto full_key = makeKey(tenant, key);
        auto it = index_.find(full_key);
        
        if (it == index_.end() || !it->second->is_valid) {
            return std::nullopt;
        }
        
        // 检查过期
        if (std::chrono::steady_clock::now() >= it->second->expire_time) {
            it->second->is_valid = false;
            index_.erase(it);
            return std::nullopt;
        }
        
        entries_.splice(entries_.begin(), entries_, it->second);
        return it->second->value;
    }
    
    void put(const std::string& tenant, const std::string& key, 
            const std::string& value, uint64_t ttl_ms) {
        auto full_key = makeKey(tenant, key);
        auto expire_time = std::chrono::steady_clock::now() + 
                          std::chrono::milliseconds(ttl_ms);
        
        auto it = index_.find(full_key);
        if (it != index_.end()) {
            it->second->value = value;
            it->second->expire_time = expire_time;
            it->second->is_valid = true;
            entries_.splice(entries_.begin(), entries_, it->second);
        } else {
            if (entries_.size() >= capacity_) {
                auto& old = entries_.back();
                old.is_valid = false;
                index_.erase(makeKey(old.tenant, old.key));
                entries_.pop_back();
            }
            
            entries_.emplace_front(key, value, tenant, expire_time);
            index_[full_key] = entries_.begin();
        }
    }
    
    void printStats() const {
        std::cout << "多租户缓存统计:\n";
        std::cout << "  总条目数: " << entries_.size() << "\n";
        std::cout << "  容量: " << capacity_ << "\n";
        
        // 统计每个租户的条目数
        std::unordered_map<std::string, int> tenant_counts;
        for (const auto& entry : entries_) {
            if (entry.is_valid) {
                tenant_counts[entry.tenant]++;
            }
        }
        
        std::cout << "  租户分布:\n";
        for (const auto& [tenant, count] : tenant_counts) {
            std::cout << "    " << tenant << ": " << count << " 条目\n";
        }
    }
};

void demo_multi_tenant_cache() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "场景3: 多租户缓存系统（可变TTL LRU）\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    MultiTenantCache cache(20);
    
    // 不同租户使用不同的TTL策略
    std::cout << "存储不同租户的数据:\n";
    
    // 免费用户：短TTL
    cache.put("tenant_free_1", "user_profile", "Free User 1", 1000);
    cache.put("tenant_free_2", "user_profile", "Free User 2", 1000);
    std::cout << "  免费租户（TTL=1s）: 2个条目\n";
    
    // 标准用户：中等TTL
    cache.put("tenant_standard_1", "config", "Standard Config", 5000);
    cache.put("tenant_standard_1", "settings", "Standard Settings", 5000);
    cache.put("tenant_standard_2", "config", "Standard Config 2", 5000);
    std::cout << "  标准租户（TTL=5s）: 3个条目\n";
    
    // 企业用户：长TTL
    cache.put("tenant_enterprise_1", "config", "Enterprise Config", 60000);
    cache.put("tenant_enterprise_1", "data", "Important Data", 60000);
    cache.put("tenant_enterprise_2", "config", "Enterprise Config 2", 60000);
    std::cout << "  企业租户（TTL=60s）: 3个条目\n";
    
    std::cout << "\n";
    cache.printStats();
    
    // 等待2秒
    std::cout << "\n等待2秒...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // 免费用户数据应该过期
    std::cout << "\n访问测试:\n";
    std::cout << "  免费用户1: " 
              << cache.get("tenant_free_1", "user_profile").value_or("已过期") << "\n";
    std::cout << "  标准用户1: " 
              << cache.get("tenant_standard_1", "config").value_or("已过期") << "\n";
    std::cout << "  企业用户1: " 
              << cache.get("tenant_enterprise_1", "config").value_or("已过期") << "\n";
}

// ============================================================================
// 场景4: API限流器（使用LRU记录请求历史）
// ============================================================================

class RateLimiter {
private:
    struct RequestHistory {
        std::string client_id;
        std::deque<std::chrono::steady_clock::time_point> timestamps;
        
        explicit RequestHistory(const std::string& id) : client_id(id) {}
    };
    
    size_t capacity_;
    size_t max_requests_;
    std::chrono::milliseconds window_;
    
    std::list<RequestHistory> histories_;
    std::unordered_map<std::string, std::list<RequestHistory>::iterator> index_;
    
public:
    RateLimiter(size_t capacity, size_t max_requests, uint64_t window_ms)
        : capacity_(capacity), max_requests_(max_requests), 
          window_(window_ms) {}
    
    bool allowRequest(const std::string& client_id) {
        auto now = std::chrono::steady_clock::now();
        auto cutoff = now - window_;
        
        auto it = index_.find(client_id);
        if (it == index_.end()) {
            // 新客户端
            if (histories_.size() >= capacity_) {
                index_.erase(histories_.back().client_id);
                histories_.pop_back();
            }
            
            histories_.emplace_front(client_id);
            auto new_it = histories_.begin();
            index_[client_id] = new_it;
            new_it->timestamps.push_back(now);
            return true;
        }
        
        // 清理过期记录
        auto& history = it->second;
        while (!history->timestamps.empty() && 
               history->timestamps.front() < cutoff) {
            history->timestamps.pop_front();
        }
        
        // 检查是否超限
        if (history->timestamps.size() >= max_requests_) {
            return false;
        }
        
        // 记录请求并移到前面
        history->timestamps.push_back(now);
        histories_.splice(histories_.begin(), histories_, it->second);
        return true;
    }
    
    void printStats(const std::string& client_id) const {
        auto it = index_.find(client_id);
        if (it == index_.end()) {
            std::cout << "客户端 " << client_id << ": 无记录\n";
            return;
        }
        
        std::cout << "客户端 " << client_id << ":\n";
        std::cout << "  窗口内请求数: " << it->second->timestamps.size() << "\n";
        std::cout << "  限制: " << max_requests_ << " 请求/" 
                  << window_.count() << "ms\n";
    }
};

void demo_rate_limiter() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "场景4: API限流器（使用LRU记录请求历史）\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    // 容量100客户端，每秒最多5次请求
    RateLimiter limiter(100, 5, 1000);
    
    std::cout << "模拟API请求:\n";
    std::string client = "client_12345";
    
    // 快速发送6次请求
    for (int i = 1; i <= 6; ++i) {
        bool allowed = limiter.allowRequest(client);
        std::cout << "  请求 " << i << ": " 
                  << (allowed ? "✓ 允许" : "✗ 限流") << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "\n";
    limiter.printStats(client);
    
    // 等待1秒后重试
    std::cout << "\n等待1秒后重试...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    bool allowed = limiter.allowRequest(client);
    std::cout << "新请求: " << (allowed ? "✓ 允许" : "✗ 限流") << "\n";
    limiter.printStats(client);
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              LRU算法实际应用场景示例                            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    // 运行各个场景演示
    demo_web_page_cache();
    demo_database_cache();
    demo_multi_tenant_cache();
    demo_rate_limiter();
    
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "所有场景演示完成！\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    return 0;
}

