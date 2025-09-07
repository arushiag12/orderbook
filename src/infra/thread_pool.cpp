#include "thread_pool.h"

ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::workerLoop, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (stop) return;
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            
            if (stop && tasks.empty()) {
                return;
            }
            
            task = std::move(tasks.front());
            tasks.pop();
        }
        
        task();
    }
}