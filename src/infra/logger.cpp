#include "logger.h"
#include <iostream>

Logger::Logger(const std::string& logDirectory) 
    : logDirectory_(logDirectory), shutdown_(false) {
    
    createLogDirectory();
    
    orderLogFile_.open(logDirectory_ + "orders.log", std::ios::app);
    tradeLogFile_.open(logDirectory_ + "trades.log", std::ios::app);
    requestLogFile_.open(logDirectory_ + "requests.log", std::ios::app);
    
    if (!orderLogFile_.is_open() || !tradeLogFile_.is_open() || !requestLogFile_.is_open()) {
        std::cerr << "Error: Failed to open one or more log files in " << logDirectory_ << std::endl;
    }
    
    loggerThread_ = std::thread(&Logger::loggerLoop, this);
}

Logger::~Logger() {
    shutdown();
}

void Logger::logOrderEvent(const OrderLog& orderLog) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (shutdown_) return;
        logQueue_.emplace(orderLog);
    }
    queueCondition_.notify_one();
}

void Logger::logTradeEvent(const TradeLog& tradeLog) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (shutdown_) return;
        logQueue_.emplace(tradeLog);
    }
    queueCondition_.notify_one();
}

void Logger::logRequestOutcome(const RequestOutcome& outcome) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (shutdown_) return;
        logQueue_.emplace(outcome);
    }
    queueCondition_.notify_one();
}

void Logger::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        shutdown_ = true;
    }
    queueCondition_.notify_all();
    
    if (loggerThread_.joinable()) {
        loggerThread_.join();
    }
    
    orderLogFile_.close();
    tradeLogFile_.close();
    requestLogFile_.close();
}

void Logger::loggerLoop() {
    while (true) {
        LogEntry entry(OrderLog{}); // Default initialization
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCondition_.wait(lock, [this] { return shutdown_ || !logQueue_.empty(); });
            
            if (shutdown_ && logQueue_.empty()) {
                return;
            }
            
            entry = std::move(logQueue_.front());
            logQueue_.pop();
        }
        
        switch (entry.type) {
            case LogEntry::ORDER_EVENT:
                writeOrderEvent(entry.orderLog);
                break;
            case LogEntry::TRADE_EVENT:
                writeTradeEvent(entry.tradeLog);
                break;
            case LogEntry::REQUEST_OUTCOME:
                writeRequestOutcome(entry.requestOutcome);
                break;
        }
    }
}

void Logger::writeOrderEvent(const OrderLog& orderLog) {
    if (orderLogFile_.is_open()) {
        orderLogFile_ << orderLog << std::endl;
        orderLogFile_.flush();
    }
}

void Logger::writeTradeEvent(const TradeLog& tradeLog) {
    if (tradeLogFile_.is_open()) {
        tradeLogFile_ << tradeLog << std::endl;
        tradeLogFile_.flush();
    }
}

void Logger::writeRequestOutcome(const RequestOutcome& outcome) {
    if (requestLogFile_.is_open()) {
        requestLogFile_ << outcome << std::endl;
        requestLogFile_.flush();
    }
}

void Logger::createLogDirectory() {
    try {
        std::filesystem::create_directories(logDirectory_);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating log directory " << logDirectory_ << ": " << e.what() << std::endl;
    }
}