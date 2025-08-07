#ifndef OUTPUT_H
#   define OUTPUT_H

#include <iostream>
#include <format>
#include "core/orderbook.h"
namespace output_orderbook {

inline std::ostream& operator<<(std::ostream& os, const Orderbook& orderbook) {

    os << "Orderbook\n";
    os << "========================================\n";

    os << "Transactions: \n";
    for (auto& transaction : orderbook.transactions) {
        os << std::format("Order Id: {0} <-> Order Id: {1}\n", std::get<0>(transaction), std::get<1>(transaction));
    }

    os << "----------------------------------------\n";    
    os << "Bids: \n";
    for (auto& bid : orderbook.bids) {
        os << std::format("Order Id: {0}, Price: {1}, Quantity: {2}\n", bid->id, bid->price, bid->quantity);
    }

    os << "----------------------------------------\n";
    os << "Asks: \n";
    for (auto& ask : orderbook.asks) {
        os << std::format("Order Id: {0}, Price: {1}, Quantity {2}\n", ask->id, ask->price, ask->quantity);
    }

    return os;
}

}
#endif