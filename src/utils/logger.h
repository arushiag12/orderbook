#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "basicdefs.h"

using namespace basicdefs;

namespace logger {

class Logger {
private:
    std::ofstream transaction_file;
    std::ofstream error_file;
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
public:
    Logger(const std::string& transaction_filename = "transactions.log", 
           const std::string& error_filename = "errors.log") {
        
        // Open transaction log file
        transaction_file.open(transaction_filename, std::ios::app);
        if (transaction_file.is_open()) {
            transaction_file << "\n=== New Trading Session Started at " 
                           << getCurrentTimestamp() << " ===\n";
            transaction_file << "Timestamp,BuyOrderID,SellOrderID,Price,Quantity,Status\n";
        }
        
        // Open error log file  
        error_file.open(error_filename, std::ios::app);
        if (error_file.is_open()) {
            error_file << "\n=== New Trading Session Started at " 
                      << getCurrentTimestamp() << " ===\n";
        }
    }
    
    ~Logger() {
        if (transaction_file.is_open()) {
            transaction_file << "=== Trading Session Ended at " 
                           << getCurrentTimestamp() << " ===\n\n";
            transaction_file.close();
        }
        if (error_file.is_open()) {
            error_file << "=== Trading Session Ended at " 
                      << getCurrentTimestamp() << " ===\n\n";
            error_file.close();
        }
    }
    
    void logTransaction(OrderId buy_order_id, OrderId sell_order_id, 
                       Price price, Quantity quantity, const std::string& status = "MATCHED") {
        if (transaction_file.is_open()) {
            transaction_file << getCurrentTimestamp() << ","
                           << buy_order_id << ","
                           << sell_order_id << ","
                           << price << ","
                           << quantity << ","
                           << status << "\n";
            transaction_file.flush();
        }
        
        // Also log to console
        std::cout << "[TRANSACTION] " << getCurrentTimestamp() 
                 << " - Order " << buy_order_id << " <-> Order " << sell_order_id
                 << " | Price: " << price << " | Qty: " << quantity 
                 << " | Status: " << status << std::endl;
    }
    
    void logError(const std::string& error_message, const std::string& context = "") {
        std::string full_message = getCurrentTimestamp() + " - ERROR: " + error_message;
        if (!context.empty()) {
            full_message += " | Context: " + context;
        }
        
        if (error_file.is_open()) {
            error_file << full_message << "\n";
            error_file.flush();
        }
        
        // Also log to console
        std::cerr << "[ERROR] " << full_message << std::endl;
    }
    
    void logInfo(const std::string& info_message) {
        std::string full_message = getCurrentTimestamp() + " - INFO: " + info_message;
        
        if (error_file.is_open()) {
            error_file << full_message << "\n";
            error_file.flush();
        }
        
        std::cout << "[INFO] " << full_message << std::endl;
    }
    
    void logOrderAdded(OrderId order_id, const std::string& order_type, 
                      const std::string& side, Price price, Quantity quantity) {
        std::string message = "Order added - ID: " + std::to_string(order_id) + 
                            " | Type: " + order_type + " | Side: " + side + 
                            " | Price: " + std::to_string(price) + 
                            " | Qty: " + std::to_string(quantity);
        logInfo(message);
    }
    
    void logOrderCancelled(OrderId order_id, const std::string& side) {
        std::string message = "Order cancelled - ID: " + std::to_string(order_id) + 
                            " | Side: " + side;
        logInfo(message);
    }
};

// Global logger instance
extern Logger* g_logger;

void initializeLogger(const std::string& transaction_file = "transactions.log",
                     const std::string& error_file = "errors.log");

void shutdownLogger();

}

#endif