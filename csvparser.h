#ifndef CSVPARSER_H
#define CSVPARSER_H

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "requests.h"
#include "orders.h"
#include "basicdefs.h"

using namespace basicdefs;
using namespace orders;
using namespace requests;

namespace csvparser {

struct OrderData {
    std::string action;      // ADD, CANCEL
    std::string order_type;  // MARKET, LIMIT
    std::string side;        // BUY, SELL
    Price price;
    Quantity quantity;
    OrderId order_id;        // for CANCEL operations
};

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

OrderData parseOrderLine(const std::string& line) {
    auto tokens = split(line, ',');
    OrderData data = {};
    
    if (tokens.size() >= 6) {
        data.action = tokens[0];
        data.order_type = tokens[1];
        data.side = tokens[2];
        data.price = tokens[3].empty() ? 0 : std::stoi(tokens[3]);
        data.quantity = tokens[4].empty() ? 0 : static_cast<Quantity>(std::stoi(tokens[4]));
        data.order_id = tokens[5].empty() ? 0 : static_cast<OrderId>(std::stoi(tokens[5]));
    }
    
    return data;
}

std::unique_ptr<Request> createRequest(const OrderData& data) {
    Side side = parseSide(data.side);
    
    if (data.action == "ADD") {
        if (data.order_type == "MARKET") {
            return std::make_unique<AddRequest<MarketOrder>>(side, data.price, data.quantity);
        } else if (data.order_type == "LIMIT") {
            return std::make_unique<AddRequest<LimitOrder>>(side, data.price, data.quantity);
        }
    } else if (data.action == "CANCEL") {
        if (data.order_type == "MARKET") {
            return std::make_unique<CancelRequest<MarketOrder>>(side, data.order_id);
        } else if (data.order_type == "LIMIT") {
            return std::make_unique<CancelRequest<LimitOrder>>(side, data.order_id);
        }
    }
    
    return nullptr;
}

void loadOrdersFromCSV(const std::string& filename, std::vector<std::unique_ptr<Request>>& requests) {
    std::ifstream file(filename);
    std::string line;
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }
    
    // Skip header line
    std::getline(file, line);
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        OrderData data = parseOrderLine(line);
        auto request = createRequest(data);
        
        if (request) {
            requests.push_back(std::move(request));
        }
    }
    
    file.close();
}

}

#endif