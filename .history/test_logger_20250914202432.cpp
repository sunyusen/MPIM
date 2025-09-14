#include "im-common/include/logger/logger.h"
#include "im-common/include/logger/log_init.h"
#include <iostream>

int main() {
    // 初始化日志系统
    mpim::logger::LogInit::InitDefault("test");
    
    // 测试各种日志级别
    LOG_TRACE("This is a trace message");
    LOG_DEBUG("This is a debug message");
    LOG_INFO("This is an info message");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");
    LOG_FATAL("This is a fatal message");
    
    // 测试格式化日志
    LOG_INFO_FMT("User " << 123 << " logged in from " << "192.168.1.1");
    LOG_ERROR_FMT("Database connection failed: " << "Connection timeout");
    
    std::cout << "Logger test completed. Check logs/test.log for file output." << std::endl;
    
    return 0;
}
