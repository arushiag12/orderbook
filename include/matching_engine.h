#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <unordered_map>
#include "order.h"
#include "request_api.h"
#include "event_api.h"
#include "orderbook.h"

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

class MatchingEngine {
public:
    
    // Constructor to set logging functions
    MatchingEngine(OrderLogger order_logger, TradeLogger trade_logger)
        : order_logger_(order_logger), trade_logger_(trade_logger) {}
    
    RequestOutcome process_request(OrderBook& book, TradingRequest&& request);

private:
    RequestOutcome submit_order(OrderBook& book, std::unique_ptr<Order> order);
    RequestOutcome cancel_order(OrderBook& book, OrdId order_id);
    RequestOutcome modify_order(OrderBook& book, OrdId order_id, Px new_price, Qty new_quantity);

    RequestOutcome match_limit_order(std::unique_ptr<Order> order, OrderBook& book);
    RequestOutcome match_market_order(std::unique_ptr<Order> order, OrderBook& book);
    
    std::pair<std::vector<Fill>, std::unique_ptr<Order>> match_against_book(std::unique_ptr<Order> incoming_order, Book& opposite_book,
                                         Handles& handles, const Symb& symbol);

    void insert_sorted(std::unique_ptr<Order> order, OrderBook& book);
    void remove_from_book(OrdId order_id, OrderBook& book);
    void update_handles_after_removal(Side side, size_t removed_index, Book& book, Handles& handles);

    bool can_match(const Order& incoming, const Order& resting) const;
    
    static bool bid_comparator(const Order& a, const Order& b);
    static bool ask_comparator(const Order& a, const Order& b);
    
    // Sequence number management
    static std::unordered_map<std::string, std::atomic<uint64_t>> order_sequences_;
    static std::unordered_map<std::string, std::atomic<uint64_t>> trade_sequences_;
    
    uint64_t get_next_order_sequence(const std::string& symbol);
    uint64_t get_next_trade_sequence(const std::string& symbol);
    
    // Logging function members
    OrderLogger order_logger_;
    TradeLogger trade_logger_;
};


#endif