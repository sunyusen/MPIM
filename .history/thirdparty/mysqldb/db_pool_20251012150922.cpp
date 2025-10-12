#include "include/db_pool.h"
#include <mysql/mysql.h>

static void set_charset_utf8mb4(MYSQL* raw) {
    if (!raw) return;
    // 尽量设置为 utf8mb4，兼容现有表
    mysql_query(raw, "set names utf8mb4");
}

MySQLPool& MySQLPool::instance() {
    static MySQLPool inst;
    return inst;
}

MySQLPool::MySQLPool() {}
MySQLPool::~MySQLPool() {
    for (auto* p : all_) {
        delete p;
    }
}

void MySQLPool::configure(size_t poolSize) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (initialized_) return; // 仅首次生效
    targetSize_ = poolSize > 0 ? poolSize : targetSize_;
}

void MySQLPool::ensureInitialized() {
    if (initialized_) return;
    // 构建 targetSize_ 个已连接的 MySQL 对象
    for (size_t i = 0; i < targetSize_; ++i) {
        MySQL* conn = new MySQL();
        if (!conn->connect()) {
            delete conn;
            continue;
        }
        set_charset_utf8mb4(conn->getConnection());
        all_.push_back(conn);
        free_.push(conn);
    }
    initialized_ = true;
}

std::shared_ptr<MySQL> MySQLPool::acquire() {
    std::unique_lock<std::mutex> lk(mutex_);
    ensureInitialized();
    cv_.wait(lk, [&]{ return !free_.empty(); });
    MySQL* raw = free_.front();
    free_.pop();
    // 包装成 shared_ptr，释放时归还
    return std::shared_ptr<MySQL>(raw, [this](MySQL* p){ this->release(p); });
}

void MySQLPool::release(MySQL* conn) {
    std::lock_guard<std::mutex> lk(mutex_);
    free_.push(conn);
    cv_.notify_one();
}


