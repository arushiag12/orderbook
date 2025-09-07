#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>
#include <filesystem>
#include "event_api.h"

class Logger {
public:
    explicit Logger(const std::string& logDirectory);
    ~Logger();
    
    void logOrderEvent(const OrderLog& orderLog);
    void logTradeEvent(const TradeLog& tradeLog);
    void logRequestOutcome(const RequestOutcome& outcome);
    void shutdown();
    
private:
    struct LogEntry {
        enum Type { ORDER_EVENT, TRADE_EVENT, REQUEST_OUTCOME };
        Type type;
        OrderLog orderLog;
        TradeLog tradeLog;
        RequestOutcome requestOutcome;
        
        LogEntry(const OrderLog& ol) : type(ORDER_EVENT), orderLog(ol) {}
        LogEntry(const TradeLog& tl) : type(TRADE_EVENT), tradeLog(tl) {}
        LogEntry(const RequestOutcome& ro) : type(REQUEST_OUTCOME), requestOutcome(ro) {}
    };
    
    std::string logDirectory_;
    std::ofstream orderLogFile_;
    std::ofstream tradeLogFile_;
    std::ofstream requestLogFile_;
    
    std::queue<LogEntry> logQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::atomic<bool> shutdown_;
    std::thread loggerThread_;
    
    void loggerLoop();
    void writeOrderEvent(const OrderLog& orderLog);
    void writeTradeEvent(const TradeLog& tradeLog);
    void writeRequestOutcome(const RequestOutcome& outcome);
    void createLogDirectory();
};

#endif