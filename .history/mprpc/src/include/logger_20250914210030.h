#pragma once
#include <iostream>
#include <string>

// 简单的日志宏定义，用于 mprpc 框架
#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl
#define LOG_WARN(msg) std::cout << "[WARN] " << msg << std::endl
#define LOG_ERROR(msg) std::cout << "[ERROR] " << msg << std::endl
#define LOG_FATAL(msg) std::cout << "[FATAL] " << msg << std::endl
