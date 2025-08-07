#include "orderbook.h"
#include <algorithm>
#include <vector>

using namespace orderbook;
using namespace orders;
using namespace my_list;
using namespace logger;

// Template specializations
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

    // Add remaining quantity to order book if partially filled or not matched
    if (order->quantity > 0) {
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

    // Market orders should not remain in the book - log if partially unfilled
    if (order->quantity > 0) {
        if (g_logger) {
            g_logger->logError("Market order partially unfilled", 
                "Order ID: " + std::to_string(order_id) + 
                ", Remaining quantity: " + std::to_string(order->quantity));
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
    std::list<std::unique_ptr<Order>>& opposite_orders = side != Side::BUY ? asks : bids;
    bool any_match = false;
    
    // Use a simple approach: collect orders to remove and remove them after iteration
    std::vector<OrderId> orders_to_remove;
    
    for (auto it = it_begin; it != it_end && order->quantity > 0; ++it) {
        // Check if price conditions are met for limit order matching
        bool canMatch = (side == Side::BUY && order->price >= (*it)->price) ||
                       (side == Side::SELL && order->price <= (*it)->price);
        
        if (canMatch) {
            any_match = true;
            Quantity matched_quantity = std::min(order->quantity, (*it)->quantity);
            Price match_price = (*it)->price; // Use the resting order's price
            
            // Record the transaction
            transactions.push_back(std::make_tuple(
                side == Side::BUY ? order->id : (*it)->id,  // Buy order ID
                side == Side::BUY ? (*it)->id : order->id   // Sell order ID
            ));
            
            // Log the transaction
            if (g_logger) {
                g_logger->logTransaction(
                    side == Side::BUY ? order->id : (*it)->id,  // Buy order ID
                    side == Side::BUY ? (*it)->id : order->id,  // Sell order ID  
                    match_price, 
                    matched_quantity
                );
            }
            
            // Update quantities
            order->quantity -= matched_quantity;
            (*it)->quantity -= matched_quantity;
            
            // Mark for removal if fully filled
            if ((*it)->quantity == 0) {
                orders_to_remove.push_back((*it)->id);
            }
        }
    }
    
    // Remove filled orders
    for (OrderId id : orders_to_remove) {
        deleteItem(id, opposite_orders);
    }
    
    return any_match;
}

bool Orderbook::matchMarket(auto it_begin, auto it_end, std::unique_ptr<Order>& order, Side side) 
{
    std::list<std::unique_ptr<Order>>& opposite_orders = side != Side::BUY ? asks : bids;
    bool any_match = false;
    
    // Use a simple approach: collect orders to remove and remove them after iteration
    std::vector<OrderId> orders_to_remove;
    
    for (auto it = it_begin; it != it_end && order->quantity > 0; ++it) {
        any_match = true;
        Quantity matched_quantity = std::min(order->quantity, (*it)->quantity);
        Price match_price = (*it)->price; // Market order gets the resting order's price
        
        // Record the transaction
        transactions.push_back(std::make_tuple(
            side == Side::BUY ? order->id : (*it)->id,  // Buy order ID
            side == Side::BUY ? (*it)->id : order->id   // Sell order ID
        ));
        
        // Log the market order transaction
        if (g_logger) {
            g_logger->logTransaction(
                side == Side::BUY ? order->id : (*it)->id,  // Buy order ID
                side == Side::BUY ? (*it)->id : order->id,  // Sell order ID
                match_price,
                matched_quantity,
                "MARKET_FILL"
            );
        }
        
        // Update quantities
        order->quantity -= matched_quantity;
        (*it)->quantity -= matched_quantity;
        
        // Mark for removal if fully filled
        if ((*it)->quantity == 0) {
            orders_to_remove.push_back((*it)->id);
        }
    }
    
    // Remove filled orders
    for (OrderId id : orders_to_remove) {
        deleteItem(id, opposite_orders);
    }
    
    return any_match;
}