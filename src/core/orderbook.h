#ifndef ORDERBOOK_H
#   define ORDERBOOK_H

#include <iostream>
#include <vector>
#include <list>
#include <memory>
#include <tuple>
# include <unordered_map>
#include <string>
#include <type_traits>
#include <format>
#include <algorithm>

#include "basicdefs.h"
#include "orders.h"
#include "list.h"
#include "../utils/logger.h"
using orders::Order;
using orders::LimitOrder;
using orders::MarketOrder;
using orders::CheckValidOrderType_v;
using orders::OrderTypes;
using my_list::addItem;
using my_list::deleteItem;
using logger::g_logger;
namespace orderbook {

struct Orderbook {

    std::list<std::unique_ptr<Order>> bids;
    std::list<std::unique_ptr<Order>> asks;
    std::vector<std::tuple<OrderId, OrderId>> transactions;

    void cancel(Side side, OrderId id);

    template <typename OrderType> 
    requires CheckValidOrderType_v<OrderType, OrderTypes>
    void add(Side side, Price p, Quantity q);

    bool matchMarket(auto it_begin, auto it_end, std::unique_ptr<Order>& order, Side side);
    bool matchLimit(auto it_begin, auto it_end, std::unique_ptr<Order>& order, Side side);
};

// Explicit template specialization declarations  
template<> void Orderbook::add<LimitOrder>(Side side, Price p, Quantity q);
template<> void Orderbook::add<MarketOrder>(Side side, Price p, Quantity q);

}
#endif
