#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <algorithm>
#include "order.h"
#include "event_api.h"

using Book = std::vector<std::unique_ptr<Order>>;
using Handles = std::unordered_map<OrdId, OrderHandle>;

struct OrderHandle {
    Side side;
    size_t vectorIndex;
};

class OrderBook {
friend class MatchingEngine;
public:
    inline explicit OrderBook(const std::string& symbol) : symbol_(symbol) {}

    std::string symbol_;
    Book bids_;  // Sorted: high price first, early time first
    Book asks_;  // Sorted: low price first, early time first
    Handles order_handles_;
};


#endif