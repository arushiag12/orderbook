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
#include "logger.h"
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

template <>
void Orderbook::add<LimitOrder>(Side side, Price p, Quantity q) 
{
    std::unique_ptr<Order> order = std::make_unique<LimitOrder>(side, p, q);
    OrderId order_id = order->id;

    if (g_logger) {
        g_logger->logOrderAdded(order_id, "LIMIT", side == Side::BUY ? "BUY" : "SELL", p, q);
    }

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
    std::unique_ptr<Order> order = std::make_unique<MarketOrder>(side, p, q);
    OrderId order_id = order->id;

    if (g_logger) {
        g_logger->logOrderAdded(order_id, "MARKET", side == Side::BUY ? "BUY" : "SELL", p, q);
    }

    bool matched = side == Side::BUY 
                ? matchMarket(asks.begin(), asks.end(), order, Side::BUY)
                : matchMarket(bids.rbegin(), bids.rend(), order, Side::SELL); 

    if (!matched) {
        std::list<std::unique_ptr<Order>>& l = side == Side::BUY ? bids : asks;
        addItem(order, l);
        if (g_logger) {
            g_logger->logError("Market order not matched", "Order ID: " + std::to_string(order_id));
        }
    }
}

void Orderbook::cancel(Side side, OrderId id) 
{
    std::list<std::unique_ptr<Order>>& l = side == Side::BUY ? bids : asks;
    
    if (g_logger) {
        g_logger->logOrderCancelled(id, side == Side::BUY ? "BUY" : "SELL");
    }
    
    deleteItem(id, l);
}

bool Orderbook::matchLimit(auto it_begin, auto it_end, std::unique_ptr<Order>& order, Side side) 
{
    std::list<std::unique_ptr<Order>>& m = side != Side::BUY ? asks : bids;
    for (auto it = it_begin; it != it_end; ++it) {
        // Check if price conditions are met for limit order matching
        bool canMatch = (side == Side::BUY && order->price >= (*it)->price) ||
                       (side == Side::SELL && order->price <= (*it)->price);
        
        if (canMatch && order->quantity == (*it)->quantity) {
            transactions.push_back(std::make_tuple(order->id, (*it)->id));
            
            // Log the transaction
            if (g_logger) {
                Price match_price = (*it)->price; // Use the resting order's price
                g_logger->logTransaction(
                    side == Side::BUY ? order->id : (*it)->id,  // Buy order ID
                    side == Side::BUY ? (*it)->id : order->id,  // Sell order ID  
                    match_price, 
                    order->quantity
                );
            }
            
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
        if (order->quantity == (*it)->quantity) {
            transactions.push_back(std::make_tuple(order->id, (*it)->id));
            
            // Log the market order transaction
            if (g_logger) {
                Price match_price = (*it)->price; // Market order gets the resting order's price
                g_logger->logTransaction(
                    side == Side::BUY ? order->id : (*it)->id,  // Buy order ID
                    side == Side::BUY ? (*it)->id : order->id,  // Sell order ID
                    match_price,
                    order->quantity,
                    "MARKET_FILL"
                );
            }
            
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
