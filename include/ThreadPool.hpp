#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threadCount);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename Function>
    auto submit(Function&& function) -> std::future<std::invoke_result_t<Function>> {
        using Result = std::invoke_result_t<Function>;
        auto task = std::make_shared<std::packaged_task<Result()>>(
            std::forward<Function>(function));
        std::future<Result> future = task->get_future();

        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (stopping_) {
                throw std::runtime_error("Cannot submit to a stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }

        condition_.notify_one();
        return future;
    }

private:
    void workerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    bool stopping_ = false;
};
