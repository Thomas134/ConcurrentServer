#include "performance_monitor.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <chrono>
#include <vector>
#include <mutex>

// 获取单例实例
PerformanceMonitor& PerformanceMonitor::GetInstance() {
    static PerformanceMonitor instance;
    return instance;
}

// 开始监控
void PerformanceMonitor::Start() {
    running_.store(true, std::memory_order_relaxed);
    start_time_ = std::chrono::steady_clock::now();
    total_requests_.store(0, std::memory_order_relaxed);
    response_times_.clear();
}

// 停止监控
void PerformanceMonitor::Stop() {
    running_.store(false, std::memory_order_relaxed);
}

// 记录请求数
void PerformanceMonitor::RecordRequest() {
    if (running_.load(std::memory_order_relaxed)) {
        total_requests_.fetch_add(1, std::memory_order_relaxed);
    }
}

// 生成性能报告
std::string PerformanceMonitor::GenerateReport() {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time_).count();

    std::ostringstream json;
    json << "{"
        << "\"duration\":" << duration << ","
        << "\"total_requests\":" << total_requests_.load() << ","
        << "\"throughput\":" << (duration > 0 ? static_cast<double>(total_requests_.load()) / duration : 0)
        << "}";

    return json.str();  // 返回 JSON 字符串
}
