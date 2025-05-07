#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>
#include <string>

class PerformanceMonitor {
public:
    static PerformanceMonitor& GetInstance();

    void Start();
    void Stop();
    void RecordRequest();
    std::string GenerateReport();

    // Ω˚”√øΩ±¥∫Õ“∆∂Ø
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

private:
    PerformanceMonitor() = default;

    std::atomic<bool> running_{ false };
    std::chrono::steady_clock::time_point start_time_;

    std::atomic<uint64_t> total_requests_{ 0 };
    std::vector<double> response_times_;

    mutable std::mutex data_mutex_;
};