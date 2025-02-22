#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace zvcr::thread_pool {

    class ThreadPool {
    public:
        explicit ThreadPool(size_t);

        ThreadPool();

        template<class F, class... Args>
        auto enqueue(F&& f, Args&&... args)
            -> std::future<std::result_of_t<F(Args...)>>;

        [[nodiscard]]
        size_t queueSize() const;

        ~ThreadPool();
    private:
        void init(size_t threads);

        std::vector< std::thread > workers;
        std::queue< std::function<void()> > tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
    };

    inline void ThreadPool::init(const size_t threads) {
        for (size_t i = 0; i<threads; i++) {
            workers.emplace_back([this] {
                for(;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(this->queue_mutex);
                        this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });

                        if (this->stop && this->tasks.empty()) return;

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    inline ThreadPool::ThreadPool(): stop(false) {
        init(std::thread::hardware_concurrency());
    }

    inline ThreadPool::ThreadPool(const size_t threads): stop(false) {
        init(threads);
    }

    template<class F, class... Args>
    auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<std::result_of_t<F(Args...)>> {
        using return_type = std::result_of_t<F(Args...)>;

        auto task = std::make_shared< std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock lock(queue_mutex);
            if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");

            tasks.emplace([task](){ (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    inline size_t ThreadPool::queueSize() const {
        return tasks.size();
    }

    inline ThreadPool::~ThreadPool() {
        {
            std::unique_lock lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers)
            worker.join();
    }

}