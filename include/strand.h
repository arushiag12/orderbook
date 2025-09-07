#ifndef STRAND_H
#define STRAND_H

#include "thread_pool.h"
#include <functional>
#include <queue>
#include <mutex>

// Strand provides serialized execution of tasks (ensures tasks run sequentially)
class Strand {
public:
    explicit Strand(ThreadPool& threadPool);
    
    // Post a task to be executed in this strand (serialized)
    void post(std::function<void()> task);
    
private:
    ThreadPool& threadPool_;
    std::queue<std::function<void()>> taskQueue_;
    std::mutex mutex_;
    std::atomic<bool> running_{false};
    
    void executeNext();
};

#endif