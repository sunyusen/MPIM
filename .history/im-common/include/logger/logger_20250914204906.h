#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <atomic>
#include <time.h>
#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <vector>
#include <sstream>

namespace mpim {
namespace logger {

enum LogLevel
{
    TRACE = 0, // 跟踪级别，最详细的日志
    DEBUG = 1, // 调试日志，记录调试信息
    INFO  = 2, // 信息级别，记录正常的操作信息
    WARN  = 3, // 警告信息，记录可能导致问题的事件
    ERROR = 4, // 错误信息
    FATAL = 5  // 致命错误，通常需要立即停止程序
};

const char *LogLevelToString(LogLevel level);

// 异步写日志的日志队列
template<typename T>
class LockQueue
{
public:
    LockQueue() : m_closed(false) {}

    // 关闭队列：唤醒所有等待者，之后的push直接失败
    void Close()
    {
        std::lock_guard<std::mutex> lg(m_mutex);
        m_closed = true;
        m_cond.notify_all();
    }

    bool IsClosed() const {
        std::lock_guard<std::mutex> lg(m_mutex);
        return m_closed;
    }

    // 多个worker线程都会写日志queue
    bool Push(const T &data)
    {
        std::lock_guard<std::mutex> lg(m_mutex);
        if(m_closed) return false;
        m_queue.push(data);
        m_cond.notify_one();
        return true;
    }

    // 右值Push
    bool Push(T&& data)
    {
        std::lock_guard<std::mutex> lg(m_mutex);
        if(m_closed) return false;
        m_queue.push(std::move(data));
        m_cond.notify_one();
        return true;
    }

    // 阻塞Pop: 返回false表示队列已关闭且为空
    bool Pop(T& out)
    {
        std::unique_lock<std::mutex> ul(m_mutex);
        m_cond.wait(ul, [this]() { return !m_queue.empty() || m_closed; });
        if(m_queue.empty()) return false;
        out = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    // 带超时的Pop
    bool TryPop(T& out, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> ul(m_mutex);
        if(!m_cond.wait_for(ul, timeout, [this]() { return !m_queue.empty() || m_closed; }))
            return false;
        if(m_queue.empty()) return false;
        out = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool Empty()
    {
        std::lock_guard<std::mutex> lg(m_mutex);
        return m_queue.empty();
    }

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_closed;
};

class ISink
{
public:
    explicit ISink(LogLevel threshold) : threshold_(threshold) {}
    virtual ~ISink() = default;
    virtual void Write(const std::string &log_message) = 0;
    virtual bool Accepts(LogLevel level) const { return level >= threshold_; }
    void SetThreshold(LogLevel level) { threshold_ = level; }
    LogLevel GetThreshold() const { return threshold_; }

private:
    LogLevel threshold_;
};

class ConsoleSink : public ISink
{
public:
    using ISink::ISink;
    void Write(const std::string &log_message) override
    {
        std::cout << log_message << std::endl;
    }
};

class FileSink : public ISink
{
private:
    std::ofstream log_file;

public:
    FileSink(const std::string &filename, LogLevel th) : 
        ISink(th),
        log_file(filename, std::ios::app)
    {}

    void Write(const std::string &log_message) override
    {
        if (log_file.is_open())
        {
            log_file << log_message << std::endl;
            log_file.flush();
        }
    }
};

struct LoggerConfig
{
    LogLevel global_threshold = INFO;
    bool enable_console = true;
    LogLevel console_level = INFO;
    bool enable_file = true;
    LogLevel file_level = DEBUG;
    std::string file_dir = "logs";
    std::string file_name = "app.log";
    bool roll_by_day = true;
    size_t roll_size_bytes = 0;
    int flush_interval_ms = 200;
    size_t queue_capacity = 100000;
};

struct LogRecord
{
    LogLevel level;
    std::string msg;
    std::string module;
    std::string function;
    int line;
};

// 日志系统主类
class Logger
{
public:
    ~Logger();
    static Logger &GetInstance();
    void Init(const LoggerConfig &config);
    void SetLogLevel(LogLevel level);
    void Log(LogLevel level, const std::string &msg, 
             const std::string &module = "", 
             const std::string &function = "", 
             int line = 0);
    bool IsEnabled(LogLevel level) const;
    void Start();
    void Stop();

private:
    Logger();
    Logger(const Logger &) = delete;
    Logger(Logger &&) = delete;
    void WriteLogTask();
    std::string FormatLogMessage(const LogRecord &record);

private:
    mutable std::mutex mu_;
    LoggerConfig cfg_;
    std::vector<std::unique_ptr<ISink>> sinks;
    LogLevel global_threshold;
    LockQueue<LogRecord> m_lckQue;
    std::atomic<bool> is_running;
    std::thread write_thread;
};

// 日志流类，支持 << 操作符
class LogStream {
public:
    LogStream(LogLevel level, const std::string& file, const std::string& func, int line)
        : level_(level), file_(file), func_(func), line_(line) {}
    
    template<typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }
    
    ~LogStream() {
        if (Logger::GetInstance().IsEnabled(level_)) {
            Logger::GetInstance().Log(level_, stream_.str(), file_, func_, line_);
        }
    }

private:
    LogLevel level_;
    std::string file_;
    std::string func_;
    int line_;
    std::ostringstream stream_;
};

// 便捷的日志宏定义
#define LOG_TRACE(msg) do { \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::TRACE, msg, __FILE__, __FUNCTION__, __LINE__); \
} while(0)

#define LOG_DEBUG(msg) do { \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::DEBUG, msg, __FILE__, __FUNCTION__, __LINE__); \
} while(0)

#define LOG_INFO(msg) do { \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::INFO, msg, __FILE__, __FUNCTION__, __LINE__); \
} while(0)

#define LOG_WARN(msg) do { \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::WARN, msg, __FILE__, __FUNCTION__, __LINE__); \
} while(0)

#define LOG_ERROR(msg) do { \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::ERROR, msg, __FILE__, __FUNCTION__, __LINE__); \
} while(0)

#define LOG_FATAL(msg) do { \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::FATAL, msg, __FILE__, __FUNCTION__, __LINE__); \
} while(0)

// 支持格式化的日志宏
#define LOG_TRACE_FMT(fmt, ...) do { \
    std::stringstream ss; \
    ss << fmt; \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::TRACE, ss.str(), __FILE__, __FUNCTION__, __LINE__); \
} while(0)

#define LOG_DEBUG_FMT(fmt, ...) do { \
    std::stringstream ss; \
    ss << fmt; \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::DEBUG, ss.str(), __FILE__, __FUNCTION__, __LINE__); \
} while(0)

#define LOG_INFO_FMT(fmt, ...) do { \
    std::stringstream ss; \
    ss << fmt; \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::INFO, ss.str(), __FILE__, __FUNCTION__, __LINE__); \
} while(0)

#define LOG_WARN_FMT(fmt, ...) do { \
    std::stringstream ss; \
    ss << fmt; \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::WARN, ss.str(), __FILE__, __FUNCTION__, __LINE__); \
} while(0)

#define LOG_ERROR_FMT(fmt, ...) do { \
    std::stringstream ss; \
    ss << fmt; \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::ERROR, ss.str(), __FILE__, __FUNCTION__, __LINE__); \
} while(0)

#define LOG_FATAL_FMT(fmt, ...) do { \
    std::stringstream ss; \
    ss << fmt; \
    mpim::logger::Logger::GetInstance().Log(mpim::logger::FATAL, ss.str(), __FILE__, __FUNCTION__, __LINE__); \
} while(0)

} // namespace logger
} // namespace mpim
