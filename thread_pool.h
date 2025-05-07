#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <memory>
#include <future>

class ThreadPool {
public:
    ThreadPool(size_t corePoolSize, size_t maxPoolSize, std::chrono::milliseconds keepAliveTime, size_t queueCapacity)
        : corePoolSize(corePoolSize),
        maxPoolSize(maxPoolSize),
        keepAliveTime(keepAliveTime),
        queueCapacity(queueCapacity),
        stop(false),
        totalThreads(0) {
        for (size_t i = 0; i < corePoolSize; ++i) {
            addWorker(true);
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (auto& thread : allThreads) {
            if (thread.joinable()) thread.join();
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");

            if (tasks.size() >= queueCapacity) {
                if (totalThreads.load() < maxPoolSize) {
                    addWorker(false);
                }
                else {
                    throw std::runtime_error("Task queue full and max threads reached");
                }
            }
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

private:
    void addWorker(bool isCore) {
        ++totalThreads;
        allThreads.emplace_back([this, isCore] {
            auto lastWorkTime = std::chrono::steady_clock::now();
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    while (!stop && tasks.empty()) {
                        if (!isCore) {
                            if (condition.wait_until(lock, lastWorkTime + keepAliveTime) == std::cv_status::timeout) {
                                if (tasks.empty() && totalThreads.load() > corePoolSize) {
                                    --totalThreads;
                                    return;
                                }
                            }
                        }
                        else {
                            condition.wait(lock);
                        }
                    }

                    if (stop && tasks.empty()) {
                        --totalThreads;
                        return;
                    }
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
                lastWorkTime = std::chrono::steady_clock::now();
            }
            });
    }

    size_t corePoolSize;
    size_t maxPoolSize;
    std::chrono::milliseconds keepAliveTime;
    size_t queueCapacity;

    std::vector<std::thread> allThreads;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::atomic<size_t> totalThreads;
};
