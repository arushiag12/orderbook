#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    void submit(std::function<void()> task);
    void shutdown();
    
protected:
    std::atomic<bool> stop;

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    
    void workerLoop();
};

#endif