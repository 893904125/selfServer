//
// Created by szy on 2025/11/24.
//

#ifndef SELFSERVER_THREADPOOL_H
#define SELFSERVER_THREADPOOL_H
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <memory>
#include <assert.h>
#include <thread>

#include "http/httpconn.h"

class ThreadPool {
public:
    explicit  ThreadPool(size_t threadCount = 8):pool_(std::make_shared<Pool>()) {
        assert(threadCount > 0);
        for (size_t i = 0; i < threadCount; i++) {
            std::thread([pool = pool_]() {
                std::unique_lock<std::mutex> locker(pool->mtx);
                while (true) {
                    if (!pool->tasks.empty()) {
                        auto task = pool->tasks.front();
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else if (pool->isClose) break;
                    else (pool->cond.wait(locker));
                }
            }).detach();
        }
    }

    ~ThreadPool() {
        if (static_cast<bool>(pool_)){
            std::unique_lock<std::mutex> lock(pool_->mtx);
            pool_->isClose = true;
        }
        pool_->cond.notify_all();
    }

    template<class F>
    void AddTask(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();
    }

private:
    //使用结构体，然后使用一个智能指针管理
    //主线程调用会使指针数++
    //防止在主线程中提前析构掉Pool，而子线程还想访问pool的变量
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool isClose = false;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
};

#endif //SELFSERVER_THREADPOOL_H