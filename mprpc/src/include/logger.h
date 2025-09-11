#pragma once
#include "lockqueue.h"
#include <string>
#include <fstream>
#include <iostream>
#include <atomic>
#include <time.h>
#include <memory>

enum LogLevel
{
	TRACE, // 跟踪级别，最详细的日志， 跟踪程序的执行路径，输出函数的入参和返回值，记录循环递归的每一步
	DEBUG, // 调试日志，记录调试信息， 记录关键变量的值，调试特定模块的行为，输出程序的状态变化
	INFO,  // 信息级别，记录正常的操作信息		记录系统启动，关闭等关键时间，记录用户操作，记录定时任务的执行结果
	WARN,  // 警告信息，记录可能导致文件的事件		配置文件缺失但使用了默认值，外部服务响应延迟但未超时
	ERROR, // 错误信息		数据库连接失败，文件读写失败，外部服务调用失败
	FATAL  // 致命错误，通常需要立即停止程序		系统的核心组件初始化失败，关键资源耗尽，数据库崩溃导致无法继续运行
};

class ISink
{ // 日志输出目标(Sink), 如控制台、文件等
public:
	// 添加explicit防止单参数构造函数被隐式转换调用
	explicit ISink(LogLevel threshold) : threshold_(threshold) {}
	virtual ~ISink() = default;
	virtual void Write(const std::string &log_message) = 0;						// 写日志
	virtual bool Accepts(LogLevel level) const { return level >= threshold_; }; // 判断该Sink是否接受该日志级别
	void SetThreshold(LogLevel level) { threshold_ = level; }
	LogLevel GetThreshold() const { return threshold_; }

private:
	LogLevel threshold_; // 每个Sink可以有自己的日志级别阈值
};

class ConsoleSink : public ISink
{ // 控制台日志输出
public:
	using ISink::ISink; // 继承构造函数，传入阈值
	void Write(const std::string &log_message) override
	{
		std::cout << log_message << std::endl;
	}
};

class FileSink : public ISink
{ // 文件日志输出
private:
	std::ofstream log_file;

public:
	FileSink(const std::string &filename, LogLevel th) : 
		ISink(th),
		log_file(filename, std::ios::app) // 修改为 log_file
		{}

	void Write(const std::string &log_message) override
	{
		if (log_file.is_open())
		{
			log_file << log_message << std::endl;
		}
	}
};

struct LoggerConfig
{
	LogLevel global_threshold; // 全局日志级别

	bool enable_console = true;	   // 是否启用控制台输出
	LogLevel console_level = LogLevel::INFO; // 控制台日志级别

	bool enable_file = true;		   // 是否启用文件输出
	LogLevel file_level = LogLevel::DEBUG; // 文件日志级别
	std::string file_dir;	//告诉文件放在哪
	std::string file_name = "app.log";		   // 日志文件目录

	// 下面的部分以后再来探索吧
	bool roll_by_day;		// 是否按天切割日志
	size_t roll_size_bytes; // 按大小切割日志的阈值，0表示不按大小切割
	int flush_interval_ms = 200;	// 刷新间隔
	size_t queue_capacity = 100000;	// 队列容量
};

struct LogRecord
{
	LogLevel level;
	std::string msg;
};

// Mprpc框架提供的日志系统
class Logger
{
public:
	~Logger();
	// 获取日志的单例
	static Logger &GetInstance();
	// 初始化配置
	void Init(const LoggerConfig &config);
	// 设置日志级别
	void SetLogLevel(LogLevel level);
	// 写日志,写入缓存队列
	void Log(LogLevel level, const std::string &msg);
	// 检查日志级别
	bool IsEnabled(LogLevel level) const;
	// 启动/停止写线程

	void Start(); // 应该仅启动写线程，不应进行join();
	void Stop();  // 应该通知线程停止，并等待线程结束
private:
	Logger();
	Logger(const Logger &) = delete;
	Logger(Logger &&) = delete;
	void WriteLogTask(); // 写日志线程函数
	std::string FormatLogMessage(LogLevel level, const std::string &msg);

private:
	mutable std::mutex mu_;
	LoggerConfig cfg_;
	std::vector<std::unique_ptr<ISink>> sinks; // 日志输出目标
	LogLevel global_threshold;				   // 记录日志级别
	LockQueue<LogRecord> m_lckQue;			   // 日志缓冲队列
	std::atomic<bool> is_running;			   // 控制写线程的运行状态
	std::thread write_thread;				   // 写线程
	std::atomic<LogLevel> min_sink_level_{LogLevel::FATAL}; // 所有Sink中最低的日志级别
};

// 定义宏
// #define LOG_INFO(logmsgformat, ...) do { \
// 	Logger& logger = Logger::GetInstance(); \
// 	logger.SetLogLevel(INFO); \
// 	char c[1024] = {0}; \
// 	snprintf(c, 1024, logmsgformat, ##__VA_ARGS__); \
// 	if (logger.m_loglevel <= INFO) { \
// 		logger.Log(c); \
// 	} \
// } while(0);

// #define LOG_INFO(logmsgformat, ...) do {\
// 	Logger& logger = Logger::GetInstance();\
// 	logger.SetLogLevel(INFO);\
// 	char c[1024] = {0};\
// 	snprintf(c, 1024, logmsgformat, ##__VA_ARGS__);\
// 	logger.Log(c);\
// } while(0);

// #define LOG_ERROR(logmsgformat, ...) do {\
// 	Logger& logger = Logger::GetInstance();\
// 	logger.SetLogLevel(ERROR);\
// 	char c[1024] = {0};\
// 	snprintf(c, 1024, logmsgformat, ##__VA_ARGS__);\
// 	logger.Log(c);\
// } while(0);

// 一个小的语法糖，C/C++宏里，行尾的反斜杠 \ 表示这行还没有结束，请把下一行也看作是当前行的一部分， 当下一行是最末尾行的时候没有换行符，那么倒数第二行的\就等不到期待的一整行