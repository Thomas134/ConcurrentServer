#include "thread_pool.h"  
#include "http_server.h"
#include "performance_monitor.h"
#include <boost/asio.hpp>
#include <iostream>
#include <chrono>

//测试并发
void test_task(boost::asio::io_context& io_context, int id) {
    auto timer = std::make_shared<boost::asio::steady_timer>(io_context);
    timer->expires_after(std::chrono::seconds(1));
    timer->async_wait([id](const boost::system::error_code&) {
        std::cout << "Task " << id
            << " is running on thread " << std::this_thread::get_id()
            << std::endl;
        });
}

int main() {
    try {
        size_t thread_pool_size = 4; // 线程池大小
        size_t core_pool_size = 2;   // 核心线程数
        size_t max_pool_size = 8;    // 最大线程数
        std::chrono::milliseconds keep_alive_time(1000);  // 线程空闲超时
        size_t queue_capacity = 16;  // 任务队列容量
        const unsigned short port = 8080;

        boost::asio::io_context io_context;

        // 捕捉 SIGINT 和 SIGTERM
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) {
            std::cout << "Signal caught, stopping io_context...\n";
            io_context.stop();
            });

        // 创建服务器实例
        HttpServer server(io_context, port);

        // 添加测试任务
        for (int i = 0; i < 10; ++i) {
            test_task(io_context, i);
        }

        // 创建线程池实例
        ThreadPool thread_pool(core_pool_size, max_pool_size, keep_alive_time, queue_capacity);


        // 启动线程池中的多个线程处理任务
        for (int i = 0; i < thread_pool_size; ++i) {
            thread_pool.enqueue([&io_context]() {
                std::cout << "Thread " << std::this_thread::get_id() << " is running io_context\n";
                io_context.run();
                });
        }

        io_context.run();

        // 启动主线程逻辑（如监听连接）
        server.Start();

        // 主线程结束后析构 thread_pool，会自动 join 所有线程
        PerformanceMonitor::GetInstance().Stop();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
