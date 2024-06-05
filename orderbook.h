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

#include "basicdefs.h"
#include "orders.h"
#include "list.h"
using orders::Order;
using orders::LimitOrder;
using orders::MarketOrder;
using orders::CheckValidOrderType_v;
using orders::OrderTypes;
using my_list::addItem;
using my_list::deleteItem;
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

template <>
void Orderbook::add<LimitOrder>(Side side, Price p, Quantity q) 
{
    std::unique_ptr<Order> order = std::make_unique<LimitOrder>(side, p, q);

    bool matched = side == Side::BUY 
                ? matchLimit(asks.begin(), asks.end(), order, Side::BUY)
                : matchLimit(bids.rbegin(), bids.rend(), order, Side::SELL); 

    if (!matched) {
        std::list<std::unique_ptr<Order>>& l = side == Side::BUY ? bids : asks;
        addItem(order, l);
    }
}

template <>
void Orderbook::add<MarketOrder>(Side side, Price p, Quantity q) 
{
    std::unique_ptr<Order> order = std::make_unique<LimitOrder>(side, p, q);

    bool matched = side == Side::BUY 
                ? matchMarket(asks.begin(), asks.end(), order, Side::BUY)
                : matchMarket(bids.rbegin(), bids.rend(), order, Side::SELL); 

    if (!matched) {
        std::list<std::unique_ptr<Order>>& l = side == Side::BUY ? bids : asks;
        addItem(order, l);
    }
}

void Orderbook::cancel(Side side, OrderId id) 
{
    std::list<std::unique_ptr<Order>>& l = side == Side::BUY ? bids : asks;
    deleteItem(id, l);
}

bool Orderbook::matchLimit(auto it_begin, auto it_end, std::unique_ptr<Order>& order, Side side) 
{
    std::list<std::unique_ptr<Order>>& m = side != Side::BUY ? asks : bids;
    for (auto it = it_begin; it != it_end; ++it) {
        if (order->quantity == (*it)->quantity) {
            transactions.push_back(std::make_tuple(order->id, (*it)->id));
            if constexpr (std::is_same_v<decltype(it_begin), std::list<std::unique_ptr<Order>>::iterator>) {
                m.erase(it);
            } else {
                m.erase((++it).base());
            }
            return true;
        } 
    }
    return false;
}

bool Orderbook::matchMarket(auto it_begin, auto it_end, std::unique_ptr<Order>& order, Side side) 
{
    std::list<std::unique_ptr<Order>>& m = side != Side::BUY ? asks : bids;
    for (auto it = it_begin; it != it_end; ++it) {
        if ((order->quantity == (*it)->quantity) && (order->price < (*it)->price)) {
            transactions.push_back(std::make_tuple(order->id, (*it)->id));
            if constexpr (std::is_same_v<decltype(it_begin), std::list<std::unique_ptr<Order>>::iterator>) {
                m.erase(it);
            } else {
                m.erase((++it).base());
            }
            return true;
        } 
    }
    return false;
}

}
#endif
