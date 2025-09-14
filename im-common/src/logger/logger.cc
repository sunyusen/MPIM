#include "logger/logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace mpim {
namespace logger {

const char *LogLevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::TRACE:
        return "TRACE";
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARN:
        return "WARN";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::FATAL:
        return "FATAL";
    default:
        return "OFF";
    }
}

// 获取日志的单例
Logger &Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

Logger::~Logger()
{
    Stop();
}

Logger::Logger() : global_threshold(INFO), is_running(false)
{
}

void Logger::Init(const LoggerConfig &config)
{
    std::lock_guard<std::mutex> lock(mu_);
    cfg_ = config;
    global_threshold = cfg_.global_threshold;
    sinks.clear();

    // 根据配置启动对应的sink
    if (config.enable_console)
    {
        sinks.push_back(std::make_unique<ConsoleSink>(config.console_level));
    }
    if (config.enable_file)
    {
        // 确保日志目录存在
        std::string cmd = "mkdir -p " + config.file_dir;
        system(cmd.c_str());
        
        std::string path = config.file_dir + "/" + config.file_name;
        sinks.push_back(std::make_unique<FileSink>(path, config.file_level));
    }
}

void Logger::Log(LogLevel level, const std::string &msg, 
                 const std::string &module, 
                 const std::string &function, 
                 int line)
{
    if (!IsEnabled(level))
        return;
    
    if (!is_running.load())
        return;
    
    LogRecord record;
    record.level = level;
    record.msg = msg;
    record.module = module;
    record.function = function;
    record.line = line;
    
    m_lckQue.Push(std::move(record));
}

bool Logger::IsEnabled(LogLevel level) const
{
    return level >= global_threshold;
}

void Logger::Start()
{
    bool expected = false;
    if(!is_running.compare_exchange_strong(expected, true)) 
        return;
    
    write_thread = std::thread(&Logger::WriteLogTask, this);
}

void Logger::Stop()
{
    bool expected = true;
    if(!is_running.compare_exchange_strong(expected, false)) 
        return;
    
    m_lckQue.Close();
    if (write_thread.joinable()) 
        write_thread.join();
}

void Logger::WriteLogTask()
{
    LogRecord rec;
    while (is_running || !m_lckQue.Empty())
    {
        if (!m_lckQue.Pop(rec)) 
            break;
        
        std::string line = FormatLogMessage(rec);
        for (auto &s : sinks)
        {
            if (s->Accepts(rec.level)) 
                s->Write(line);
        }
    }
}

std::string Logger::FormatLogMessage(const LogRecord &record)
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto time_t = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &time_t);
#else
    localtime_r(&time_t, &tm_buf);
#endif
    
    std::stringstream ss;
    ss << "[" << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    ss << "][" << LogLevelToString(record.level) << "]";
    
    if (!record.module.empty())
    {
        ss << "[" << record.module << "]";
    }
    
    if (!record.function.empty())
    {
        ss << "[" << record.function;
        if (record.line > 0)
        {
            ss << ":" << record.line;
        }
        ss << "]";
    }
    
    ss << " " << record.msg;
    
    return ss.str();
}

void Logger::SetLogLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(mu_);
    global_threshold = level;
}

} // namespace logger
} // namespace mpim
