#include "strand.h"

Strand::Strand(ThreadPool& threadPool) : threadPool_(threadPool) {}

void Strand::post(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mutex_);
    taskQueue_.push(std::move(task));
    
    // If not currently running, start execution
    if (!running_.exchange(true)) {
        threadPool_.submit([this]() { executeNext(); });
    }
}

void Strand::executeNext() {
    std::function<void()> task;
    
    // Get next task
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (taskQueue_.empty()) {
            running_ = false;
            return;
        }
        task = std::move(taskQueue_.front());
        taskQueue_.pop();
    }
    
    // Execute the task
    try {
        task();
    } catch (...) {
        // Log error in production code
    }
    
    // Check for more tasks
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!taskQueue_.empty()) {
            // Submit next execution
            threadPool_.submit([this]() { executeNext(); });
        } else {
            running_ = false;
        }
    }
}