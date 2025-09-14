#pragma once
#include <string>
#include <map>
#include <chrono>
#include <atomic>

namespace mpim {
namespace health {

struct ServiceStatus {
    std::string service_name;
    bool is_healthy;
    std::string status_message;
    std::chrono::system_clock::time_point last_check;
    double response_time_ms;
};

class HealthChecker {
public:
    static HealthChecker& GetInstance();
    
    // 注册服务健康检查
    void RegisterService(const std::string& service_name, 
                        std::function<bool()> check_func);
    
    // 检查所有服务状态
    std::map<std::string, ServiceStatus> CheckAllServices();
    
    // 检查单个服务
    ServiceStatus CheckService(const std::string& service_name);
    
    // 获取系统整体健康状态
    bool IsSystemHealthy();

private:
    std::map<std::string, std::function<bool()>> health_checks_;
    std::mutex mutex_;
};

} // namespace health
} // namespace mpim
