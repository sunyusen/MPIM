#include "logger.h"

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
	Stop(); // 确保写线程停止
}

Logger::Logger() : global_threshold(INFO), is_running(false)
{
}

void Logger::Init(const LoggerConfig &config)
{
	std::lock_guard<std::mutex> lock(mu_); // 确保线程安全
	cfg_ = config;
	global_threshold = cfg_.global_threshold;	// 设置全局日志级别
	sinks.clear(); // 清空已有的sink

	// 根据配置启动对应的sink
	if (config.enable_console)
	{
		sinks.push_back(std::make_unique<ConsoleSink>(config.console_level));
	}
	if (config.enable_file)
	{
		std::string path = config.file_dir + "/" + config.file_name;
		sinks.push_back(std::make_unique<FileSink>(path, config.file_level));
	}
}

void Logger::Log(LogLevel level, const std::string &msg)
{
	if (!IsEnabled(level))
		return; // 日志级别不够，直接返回
	// 如果不加上load(),编译器会尝试通过隐式类型转换来读取is_running，可能会引发竞态
	if (!is_running.load())
		return;					 // 日志系统未启动，直接返回
	m_lckQue.Push({level, msg}); // 将日志记录放入队列
}

bool Logger::IsEnabled(LogLevel level) const
{
	return level >= global_threshold; // 根据全局级别判断是否允许输出
}

void Logger::Start()
{
	bool expected = false;
	// compare_exchange_strong: 原子操作
	// 如果is_running的当前值等于expected
	// 则将其设置为true，并返回true，否则返回false
	// 并更新expected为is_running的当前值
	if(!is_running.compare_exchange_strong(expected, true)) return; // 已经在运行，直接返回
	write_thread = std::thread(&Logger::WriteLogTask, this); // 启动写线程
}

void Logger::Stop()
{
	bool expected = true;
	if(!is_running.compare_exchange_strong(expected, false)) return; // 已经停止，直接返回
	m_lckQue.Close(); // 关闭队列，唤醒写线程
	if (write_thread.joinable()) write_thread.join(); // 等待写线程结束
}

void Logger::WriteLogTask()
{
	LogRecord rec;
	while (is_running || !m_lckQue.Empty())
	{ // 只要日志还在运行或者已经要求停止
		if (!m_lckQue.Pop(rec)) break;
		std::string line = FormatLogMessage(rec.level, rec.msg);
		for (auto &s : sinks)
		{
			if (s->Accepts(rec.level)) s->Write(line);
		}
	}
}

std::string Logger::FormatLogMessage(LogLevel level, const std::string &msg)
{
	using namespace std::chrono;
	auto now = system_clock::now();
	std::time_t t = system_clock::to_time_t(now);
	std::tm tm_buf{};
#if defined(_WIN32)
	localtime_s(&tm_buf, &t);
#else
	localtime_r(&t, &tm_buf);
#endif
	char timebuf[32];
	std::snprintf(timebuf, sizeof(timebuf), "%04d-%02d-%02d %02d:%02d:%02d",
				  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
				  tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
	return "[" + std::string(timebuf) + "][" + LogLevelToString(level) + "] " + msg;
}

void Logger::SetLogLevel(LogLevel level)
{
	std::lock_guard<std::mutex> lock(mu_);
	global_threshold = level;
}