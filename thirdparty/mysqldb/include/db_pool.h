#pragma once
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include "db.h"

// 简单的 MySQL 连接池（线程安全）
class MySQLPool {
public:
    // 获取单例
    static MySQLPool& instance();

    // 配置连接池大小（可在首次使用前调用，不调用则使用默认大小）
    void configure(size_t poolSize);

    // 获取一个连接（shared_ptr 带自定义删除器，归还到池）
    std::shared_ptr<MySQL> acquire();

private:
    MySQLPool();
    ~MySQLPool();
    MySQLPool(const MySQLPool&) = delete;
    MySQLPool& operator=(const MySQLPool&) = delete;

    void ensureInitialized();
    void release(MySQL* conn);

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<MySQL*> free_;     // 空闲连接
    std::vector<MySQL*> all_;     // 所有连接，析构时统一清理
    size_t targetSize_ = 8;       // 默认池大小（可按需调整/从配置读取）
    bool initialized_ = false;
};


