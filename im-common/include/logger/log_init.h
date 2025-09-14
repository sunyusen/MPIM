#pragma once
#include "logger/logger.h"

namespace mpim {
namespace logger {

class LogInit
{
public:
    static void InitDefault(const std::string &service_name = "mpim")
    {
        LoggerConfig config;
        config.global_threshold = INFO;
        config.enable_console = true;
        config.console_level = INFO;
        config.enable_file = true;
        config.file_level = DEBUG;
        config.file_dir = "logs";
        config.file_name = service_name + ".log";
        config.roll_by_day = true;
        config.flush_interval_ms = 200;
        config.queue_capacity = 100000;
        
        auto &logger = Logger::GetInstance();
        logger.Init(config);
        logger.Start();
        
        LOG_INFO_STR("Logger initialized for service: " + service_name);
    }
    
    static void InitDebug(const std::string &service_name = "mpim")
    {
        LoggerConfig config;
        config.global_threshold = DEBUG;
        config.enable_console = true;
        config.console_level = DEBUG;
        config.enable_file = true;
        config.file_level = TRACE;
        config.file_dir = "logs";
        config.file_name = service_name + ".log";
        config.roll_by_day = true;
        config.flush_interval_ms = 100;
        config.queue_capacity = 100000;
        
        auto &logger = Logger::GetInstance();
        logger.Init(config);
        logger.Start();
        
        LOG_DEBUG_STR("Debug logger initialized for service: " + service_name);
    }
    
    static void InitRelease(const std::string &service_name = "mpim")
    {
        LoggerConfig config;
        config.global_threshold = INFO;
        config.enable_console = false;
        config.enable_file = true;
        config.file_level = INFO;
        config.file_dir = "logs";
        config.file_name = service_name + ".log";
        config.roll_by_day = true;
        config.flush_interval_ms = 500;
        config.queue_capacity = 50000;
        
        auto &logger = Logger::GetInstance();
        logger.Init(config);
        logger.Start();
        
        LOG_INFO_STR("Release logger initialized for service: " + service_name);
    }
};

} // namespace logger
} // namespace mpim
