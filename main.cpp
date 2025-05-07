#include "thread_pool.h"  
#include "http_server.h"
#include "performance_monitor.h"
#include <boost/asio.hpp>
#include <iostream>
#include <chrono>

//���Բ���
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
        size_t thread_pool_size = 4; // �̳߳ش�С
        size_t core_pool_size = 2;   // �����߳���
        size_t max_pool_size = 8;    // ����߳���
        std::chrono::milliseconds keep_alive_time(1000);  // �߳̿��г�ʱ
        size_t queue_capacity = 16;  // �����������
        const unsigned short port = 8080;

        boost::asio::io_context io_context;

        // ��׽ SIGINT �� SIGTERM
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) {
            std::cout << "Signal caught, stopping io_context...\n";
            io_context.stop();
            });

        // ����������ʵ��
        HttpServer server(io_context, port);

        // ��Ӳ�������
        for (int i = 0; i < 10; ++i) {
            test_task(io_context, i);
        }

        // �����̳߳�ʵ��
        ThreadPool thread_pool(core_pool_size, max_pool_size, keep_alive_time, queue_capacity);


        // �����̳߳��еĶ���̴߳�������
        for (int i = 0; i < thread_pool_size; ++i) {
            thread_pool.enqueue([&io_context]() {
                std::cout << "Thread " << std::this_thread::get_id() << " is running io_context\n";
                io_context.run();
                });
        }

        io_context.run();

        // �������߳��߼�����������ӣ�
        server.Start();

        // ���߳̽��������� thread_pool�����Զ� join �����߳�
        PerformanceMonitor::GetInstance().Stop();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
