#ifndef CSVPARSER_SIMPLE_H
#define CSVPARSER_SIMPLE_H

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "requests.h"
#include "orders.h"
#include "basicdefs.h"
#include "logger.h"

using namespace basicdefs;
using namespace orders;
using namespace requests;
using namespace logger;

namespace csvparser_simple {

std::vector<std::string> split(const std::string& line, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

Side parseSide(const std::string& side_str) {
    return (side_str == "BUY") ? Side::BUY : Side::SELL;
}

void loadOrdersFromCSV(const std::string& filename, std::vector<std::unique_ptr<Request>>& requests) {
    std::ifstream file(filename);
    std::string line;
    int line_number = 0;
    int successful_orders = 0;
    int failed_orders = 0;
    
    if (!file.is_open()) {
        std::string error_msg = "Could not open file " + filename;
        std::cerr << "Error: " << error_msg << std::endl;
        if (g_logger) {
            g_logger->logError(error_msg);
        }
        return;
    }
    
    if (g_logger) {
        g_logger->logInfo("Started loading orders from " + filename);
    }
    
    // Skip header line
    std::getline(file, line);
    line_number++;
    
    while (std::getline(file, line)) {
        line_number++;
        if (line.empty()) continue;
        
        try {
            auto tokens = split(line, ',');
            if (tokens.size() < 6) {
                std::string error_msg = "Insufficient columns in line " + std::to_string(line_number);
                if (g_logger) {
                    g_logger->logError(error_msg, "Expected 6 columns, got " + std::to_string(tokens.size()));
                }
                failed_orders++;
                continue;
            }
            
            std::string action = tokens[0];
            std::string order_type = tokens[1];
            std::string side_str = tokens[2];
            
            // Validate action
            if (action != "ADD" && action != "CANCEL") {
                std::string error_msg = "Invalid action '" + action + "' at line " + std::to_string(line_number);
                if (g_logger) {
                    g_logger->logError(error_msg, "Valid actions: ADD, CANCEL");
                }
                failed_orders++;
                continue;
            }
            
            // Validate order type
            if (order_type != "MARKET" && order_type != "LIMIT") {
                std::string error_msg = "Invalid order_type '" + order_type + "' at line " + std::to_string(line_number);
                if (g_logger) {
                    g_logger->logError(error_msg, "Valid types: MARKET, LIMIT");
                }
                failed_orders++;
                continue;
            }
            
            // Validate side
            if (side_str != "BUY" && side_str != "SELL") {
                std::string error_msg = "Invalid side '" + side_str + "' at line " + std::to_string(line_number);
                if (g_logger) {
                    g_logger->logError(error_msg, "Valid sides: BUY, SELL");
                }
                failed_orders++;
                continue;
            }
            
            int price = tokens[3].empty() ? 0 : std::stoi(tokens[3]);
            unsigned int quantity = tokens[4].empty() ? 0 : static_cast<unsigned int>(std::stoi(tokens[4]));
            unsigned int order_id = tokens[5].empty() ? 0 : static_cast<unsigned int>(std::stoi(tokens[5]));
            
            // Validate numeric fields
            if (action == "ADD" && (price <= 0 || quantity <= 0)) {
                std::string error_msg = "Invalid price/quantity for ADD at line " + std::to_string(line_number);
                if (g_logger) {
                    g_logger->logError(error_msg, "Price: " + std::to_string(price) + ", Quantity: " + std::to_string(quantity));
                }
                failed_orders++;
                continue;
            }
            
            if (action == "CANCEL" && order_id == 0) {
                std::string error_msg = "Invalid order_id for CANCEL at line " + std::to_string(line_number);
                if (g_logger) {
                    g_logger->logError(error_msg, "Order ID cannot be 0 for CANCEL operations");
                }
                failed_orders++;
                continue;
            }
            
            Side side = parseSide(side_str);
            
            // Create requests
            if (action == "ADD") {
                if (order_type == "MARKET") {
                    requests.push_back(std::make_unique<AddRequest<MarketOrder>>(side, price, quantity));
                } else if (order_type == "LIMIT") {
                    requests.push_back(std::make_unique<AddRequest<LimitOrder>>(side, price, quantity));
                }
                successful_orders++;
            } else if (action == "CANCEL") {
                if (order_type == "MARKET") {
                    requests.push_back(std::make_unique<CancelRequest<MarketOrder>>(side, order_id));
                } else if (order_type == "LIMIT") {
                    requests.push_back(std::make_unique<CancelRequest<LimitOrder>>(side, order_id));
                }
                successful_orders++;
            }
            
        } catch (const std::exception& e) {
            std::string error_msg = "Exception parsing line " + std::to_string(line_number) + ": " + e.what();
            if (g_logger) {
                g_logger->logError(error_msg, "Line content: " + line);
            }
            failed_orders++;
        }
    }
    
    file.close();
    
    if (g_logger) {
        std::string summary = "CSV parsing completed. Successful: " + std::to_string(successful_orders) + 
                            ", Failed: " + std::to_string(failed_orders) + 
                            ", Total lines: " + std::to_string(line_number);
        g_logger->logInfo(summary);
    }
}

}

#endif