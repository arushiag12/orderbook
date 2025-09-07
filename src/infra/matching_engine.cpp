#include "matching_engine.h"
#include <algorithm>
#include <chrono>

// Static member definitions
std::unordered_map<std::string, std::atomic<uint64_t>> MatchingEngine::order_sequences_;
std::unordered_map<std::string, std::atomic<uint64_t>> MatchingEngine::trade_sequences_;

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

RequestOutcome MatchingEngine::process_request(OrderBook& book, TradingRequest&& tr) {
    auto request_visitor = Overloaded{
        [this, &book](NewOrderRequest&& r) -> RequestOutcome { 
            auto order = create_order(r.order_type, r.params);
            if (!order) {
                RequestOutcome outcome{
                    .request_id = r.request_id,
                    .status = RequestStatus::REJECTED,
                    .reason = RejectReason::INVALID_PRICE,
                    .message = "Invalid order type: " + r.order_type
                };
                
                // Generate OrderLog event for REJECTED
                OrderLog order_log{
                    .symbol = "UNKNOWN", // We don't know symbol at this point
                    .seq = get_next_order_sequence("UNKNOWN"),
                    .ts = std::chrono::steady_clock::now(),
                    .type = OrderEventType::REJECTED,
                    .order_id = 0, // No valid order ID
                    .reason = RejectReason::INVALID_PRICE
                };
                order_logger_(order_log);
                
                return outcome;
            }
            auto outcome = submit_order(book, std::move(order));
            outcome.request_id = r.request_id;
            return outcome;
        },
        [this, &book](CancelOrderRequest&& r) -> RequestOutcome {
            auto outcome = cancel_order(book, r.order_id);
            outcome.request_id = r.request_id;
            return outcome;
        },
        [this, &book](ModifyOrderRequest&& r) -> RequestOutcome {
            auto outcome = modify_order(book, r.order_id, r.new_price, r.new_quantity);
            outcome.request_id = r.request_id;
            return outcome;
        }
    };
    
    return std::visit(request_visitor, std::move(tr)); 
}

RequestOutcome MatchingEngine::submit_order(OrderBook& book, std::unique_ptr<Order> order) {
    auto order_visitor = Overloaded{
        [this, &book](MarketOrder& ord) -> RequestOutcome {
            auto order_ptr = std::make_unique<Order>(ord);
            return match_market_order(std::move(order_ptr), book);
        },
        [this, &book](LimitOrder& ord) -> RequestOutcome {
            auto order_ptr = std::make_unique<Order>(ord);
            return match_limit_order(std::move(order_ptr), book);
        }
    };
    return std::visit(order_visitor, *order);
}

RequestOutcome MatchingEngine::cancel_order(OrderBook& book, OrdId orderId) {
    auto& handles = book.order_handles_;
    auto handleIt = handles.find(orderId);
    if (handleIt == handles.end()) {
        RequestOutcome outcome;
        outcome.status = RequestStatus::REJECTED;
        outcome.reason = RejectReason::UNKNOWN_ORDER;
        outcome.message = "Order not found";
        return outcome;
    }
    
    const OrderHandle& handle = handleIt->second;
    auto& bookSide = (handle.side == Side::BUY) ? book.bids_ : book.asks_;

    if (handle.vectorIndex < bookSide.size()) {
        // Get order info before removal
        auto order_meta = std::visit([](const auto& ord) { return ord.meta; }, *bookSide[handle.vectorIndex]);
        
        // Update order status using visitor pattern
        std::visit([](auto& order) { 
            order.meta.state = OrdState::CANCELLED; 
        }, *bookSide[handle.vectorIndex]);
        
        // Generate OrderLog event for CANCELED
        OrderLog order_log;
        order_log.symbol = book.symbol_;
        order_log.seq = get_next_order_sequence(book.symbol_);
        order_log.ts = std::chrono::steady_clock::now();
        order_log.type = OrderEventType::CANCELED;
        order_log.order_id = order_meta.order_id;
        order_log.side = order_meta.side;
        order_log.price = order_meta.price;
        order_log.remaining_qty = order_meta.remaining_quantity;
        order_logger_(order_log);
        
        bookSide.erase(bookSide.begin() + handle.vectorIndex);
        update_handles_after_removal(handle.side, handle.vectorIndex, bookSide, handles);
        handles.erase(handleIt);
        
        RequestOutcome outcome;
        outcome.status = RequestStatus::OK;
        outcome.message = "Order cancelled successfully";
        return outcome;
    }
    
    RequestOutcome outcome;
    outcome.status = RequestStatus::REJECTED;
    outcome.reason = RejectReason::UNKNOWN_ORDER;
    outcome.message = "Order not found in book";
    return outcome;
}

RequestOutcome MatchingEngine::modify_order(OrderBook& book, OrdId orderId, Px newPx, Qty newQty) {
    auto& handles = book.order_handles_;
    
    auto handleIt = handles.find(orderId);
    if (handleIt == handles.end()) {
        return RequestOutcome{
            .request_id = orderId,
            .status = RequestStatus::REJECTED,
            .reason = RejectReason::UNKNOWN_ORDER,
            .message = "Order not found"
        };
    }
    
    const OrderHandle& handle = handleIt->second;
    auto& bookSide = (handle.side == Side::BUY) ? book.bids_ : book.asks_;
    auto order = &bookSide[handle.vectorIndex];
    
    // Get original order info
    auto original_kName = std::visit([](const auto& ord) { return ord.kName; }, **order);
    auto original_meta = std::visit([](const auto& ord) { return ord.meta; }, **order);

    // Create new order parameters
    NewOrderParams new_params{
        .id = original_meta.order_id,
        .client = original_meta.client_id,
        .side = original_meta.side,
        .price = newPx,
        .qty = newQty
    };
    auto new_order = create_order(original_kName, new_params);
    
    // Generate OrderLog event for REPLACED
    OrderLog order_log{
        .symbol = book.symbol_,
        .seq = get_next_order_sequence(book.symbol_),
        .ts = std::chrono::steady_clock::now(),
        .type = OrderEventType::REPLACED,
        .order_id = original_meta.order_id,
        .side = original_meta.side,
        .price = newPx,
        .remaining_qty = newQty
    };
    order_logger_(order_log);
    
    // Remove old order and resubmit the modified order
    remove_from_book(orderId, book);
    auto result = submit_order(book, std::move(new_order));
    result.message = "Order modified: " + result.message;
    return result;
}

uint64_t MatchingEngine::get_next_order_sequence(const std::string& symbol) {
    return ++order_sequences_[symbol];
}

uint64_t MatchingEngine::get_next_trade_sequence(const std::string& symbol) {
    return ++trade_sequences_[symbol];
}

RequestOutcome MatchingEngine::match_limit_order(std::unique_ptr<Order> order_ptr, OrderBook& book) {
    const auto& order = std::visit([](const auto& ord) -> const auto& { return ord; }, *order_ptr);
    
    // Get the opposite book for matching
    auto& opposite_book = (order.meta.side == Side::BUY) ? book.asks_ : book.bids_;
    
    // Match against opposite book - order_ptr will be moved inside
    auto [fills, remaining_order] = match_against_book(std::move(order_ptr), opposite_book, book.order_handles_, book.symbol_);
    
    // Calculate summary quantities
    Qty filled_qty = 0;
    for (const auto& fill : fills) {
        filled_qty += fill.qty;
    }
    
    Qty remaining_qty = 0;
    if (remaining_order) {
        remaining_qty = std::visit([](const auto& ord) -> Qty {
            return ord.meta.remaining_quantity;
        }, *remaining_order);
    }
    
    RequestOutcome outcome;
    outcome.status = RequestStatus::OK;
    outcome.fills = std::move(fills);
    outcome.taker_filled_qty = filled_qty;
    outcome.taker_remaining_qty = remaining_qty;
    
    // Check if order is fully filled
    if (remaining_qty == 0) {
        outcome.message = "Limit order fully filled";
    } else {
        // Add remaining quantity to book
        insert_sorted(std::move(remaining_order), book);
        outcome.message = "Limit order partially filled and added to book";
    }
    
    return outcome;
}

RequestOutcome MatchingEngine::match_market_order(std::unique_ptr<Order> order_ptr, OrderBook& book) {
    // Market orders must execute immediately or be rejected
    const auto& order = std::visit([](const auto& ord) -> const auto& { return ord; }, *order_ptr);
    auto& opposite_book = (order.meta.side == Side::BUY) ? book.asks_ : book.bids_;
    
    if (opposite_book.empty()) {
        RequestOutcome outcome;
        outcome.status = RequestStatus::REJECTED;
        outcome.reason = RejectReason::BOOK_CLOSED;
        outcome.message = "No liquidity available for market order";
        return outcome;
    }
    
    auto [fills, remaining_order] = match_against_book(std::move(order_ptr), opposite_book, book.order_handles_, book.symbol_);
    
    // Calculate summary quantities
    Qty filled_qty = 0;
    for (const auto& fill : fills) {
        filled_qty += fill.qty;
    }
    
    Qty remaining_qty = 0;
    if (remaining_order) {
        remaining_qty = std::visit([](const auto& ord) -> Qty {
            return ord.meta.remaining_quantity;
        }, *remaining_order);
    }
    
    RequestOutcome outcome;
    outcome.fills = std::move(fills);
    outcome.taker_filled_qty = filled_qty;
    outcome.taker_remaining_qty = remaining_qty;
    
    if (remaining_qty == 0) {
        outcome.status = RequestStatus::OK;
        outcome.message = "Market order fully filled";
    } else {
        outcome.status = RequestStatus::REJECTED;
        outcome.reason = RejectReason::BOOK_CLOSED;
        outcome.message = "Market order partially filled - insufficient liquidity";
    }
    
    return outcome;
}

std::pair<std::vector<Fill>, std::unique_ptr<Order>> 
MatchingEngine::match_against_book(std::unique_ptr<Order> incoming_order, Book& opposite_book, Handles& handles, const Symb& symbol) {
    static std::atomic<uint64_t> match_sequence{1000};
    std::vector<Fill> fills;
    bool incoming_fully_filled = false;
    
    auto it = opposite_book.begin();
    while (it != opposite_book.end()) {
        auto& resting_order = *it;
        
        bool can_match_orders = std::visit(
            [this](const auto& incoming, const auto& resting) -> bool {
                return can_match(Order(incoming), Order(resting));
            }, 
            *incoming_order, 
            *resting_order);
        
        if (!can_match_orders) break;
        
        auto incoming = std::visit([](auto& ord) { return &ord.meta; }, *incoming_order);
        auto resting = std::visit([](auto& ord) { return &ord.meta; }, *resting_order);

        if (incoming->remaining_quantity == 0) break;

        // Create fill using new Fill struct
        Fill fill{
            .symbol = symbol,
            .taker_id = incoming->order_id,
            .maker_id = resting->order_id,
            .price = resting->price,
            .qty = std::min(incoming->remaining_quantity, resting->remaining_quantity),
            .taker_is_buy = (incoming->side == Side::BUY),
            .ts = std::chrono::steady_clock::now(),
            .match_seq = ++match_sequence
        };
        fills.emplace_back(fill);
        
        // Generate TradeLog event
        TradeLog trade_log{
            .symbol = symbol,
            .seq = get_next_trade_sequence(symbol),
            .ts = fill.ts,
            .fill = fill
        };
        trade_logger_(trade_log);
        
        // Update quantities
        incoming->remaining_quantity -= fill.qty;
        resting->remaining_quantity -= fill.qty;
        
        // Generate OrderLog events for resting order
        OrderLog resting_log{
            .symbol = symbol,
            .seq = get_next_order_sequence(symbol),
            .ts = fill.ts,
            .order_id = resting->order_id,
            .side = resting->side,
            .price = resting->price,
            .remaining_qty = resting->remaining_quantity
        };
        // Check if resting order is fully filled
        if (resting->remaining_quantity == 0) {
            resting->state = OrdState::FILLED;
            resting_log.type = OrderEventType::FILLED;
            order_logger_(resting_log);
            
            handles.erase(resting->order_id);
            it = opposite_book.erase(it);
            update_handles_after_removal(resting->side, std::distance(opposite_book.begin(), it), opposite_book, handles);
        } else {
            resting->state = OrdState::PARTIALLY_FILLED;
            resting_log.type = OrderEventType::PARTIALLY_FILLED;
            order_logger_(resting_log);
            ++it;
        }
        
        // Generate OrderLog event for incoming order
        OrderLog incoming_log{
            .symbol = symbol,
            .seq = get_next_order_sequence(symbol),
            .ts = fill.ts,
            .order_id = incoming->order_id,
            .side = incoming->side,
            .price = incoming->price,
            .remaining_qty = incoming->remaining_quantity
        };
        
        if (incoming->remaining_quantity == 0) {
            incoming->state = OrdState::FILLED;
            incoming_log.type = OrderEventType::FILLED;
            incoming_fully_filled = true;
        } else {
            incoming->state = OrdState::PARTIALLY_FILLED;
            incoming_log.type = OrderEventType::PARTIALLY_FILLED;
        }
        order_logger_(incoming_log);
    }

    // Return both fills and remaining order (or nullptr if fully filled)
    return {std::move(fills), incoming_fully_filled ? std::move(incoming_order) : nullptr};
}

void MatchingEngine::insert_sorted(std::unique_ptr<Order> order, OrderBook& book) {
    auto meta = std::visit([](const auto& ord) { return ord.meta; }, *order);
    auto& book_side = (meta.side == Side::BUY) ? book.bids_ : book.asks_;
    auto comparator = (meta.side == Side::BUY) ? bid_comparator : ask_comparator;

    auto insert_pos = std::lower_bound(book_side.begin(), book_side.end(), order, comparator);
    size_t insert_index = std::distance(book_side.begin(), insert_pos);
 
    // Create and store the handle
    OrderHandle handle;
    handle.side = meta.side;
    handle.vectorIndex = insert_index;
    book.order_handles_[meta.order_id] = handle;
    
    // Update indices for orders after insertion point
    for (size_t i = insert_index + 1; i < book_side.size(); ++i) {
        OrdId id = std::visit([](const auto& ord) { return ord.meta.order_id; }, *book_side[i]);
        book.order_handles_[id].vectorIndex = i;
    }

    // Generate OrderLog event for NEW_ACCEPTED
    OrderLog order_log;
    order_log.symbol = book.symbol_;
    order_log.seq = get_next_order_sequence(book.symbol_);
    order_log.ts = std::chrono::steady_clock::now();
    order_log.type = OrderEventType::NEW_ACCEPTED;
    order_log.order_id = meta.order_id;
    order_log.side = meta.side;
    order_log.price = meta.price;
    order_log.remaining_qty = meta.remaining_quantity;
    order_logger_(order_log);

    // Insert the order into the book
    book_side.insert(insert_pos, std::move(order));
}

void MatchingEngine::remove_from_book(OrdId order_id, OrderBook& book) {
    auto& handles = book.order_handles_;
    
    auto handle_it = handles.find(order_id);
    if (handle_it == handles.end()) {
        return;
    }
    
    const OrderHandle& handle = handle_it->second;
    auto& book_side = (handle.side == Side::BUY) ? book.bids_ : book.asks_;
    
    if (handle.vectorIndex < book_side.size()) {
        book_side.erase(book_side.begin() + handle.vectorIndex);
        update_handles_after_removal(handle.side, handle.vectorIndex, book_side, handles);
        handles.erase(handle_it);
    }
}

void MatchingEngine::update_handles_after_removal(Side side, size_t removed_index, Book& book_side, Handles& handles) {
    for (size_t i = removed_index; i < book_side.size(); ++i) {
        OrdId id = std::visit([](const auto& ord) { return ord.meta.order_id; }, *book_side[i]);
        handles[id].vectorIndex = i;
    }
}

bool MatchingEngine::can_match(const Order& incoming, const Order& resting) const {
    return std::visit([](const auto& inc, const auto& rest) -> bool {
        if (inc.meta.side == rest.meta.side) return false;

        if (inc.meta.side == Side::BUY) {
            return inc.meta.price >= rest.meta.price;
        } else {
            return inc.meta.price <= rest.meta.price;
        }
    }, incoming, resting);
}

bool MatchingEngine::bid_comparator(const Order& a, const Order& b) {
    auto price_a = std::visit([](const auto& ord) { return ord.meta.price; }, a);
    auto price_b = std::visit([](const auto& ord) { return ord.meta.price; }, b);
    auto time_a = std::visit([](const auto& ord) { return ord.meta.timestamp; }, a);
    auto time_b = std::visit([](const auto& ord) { return ord.meta.timestamp; }, b);

    if (price_a != price_b) {
        return price_a > price_b;  // Higher price first for bids
    }
    return time_a < time_b;  // Earlier time first
}

bool MatchingEngine::ask_comparator(const Order& a, const Order& b) {
    auto price_a = std::visit([](const auto& ord) { return ord.meta.price; }, a);
    auto price_b = std::visit([](const auto& ord) { return ord.meta.price; }, b);
    auto time_a = std::visit([](const auto& ord) { return ord.meta.timestamp; }, a);
    auto time_b = std::visit([](const auto& ord) { return ord.meta.timestamp; }, b);

    if (price_a != price_b) {
        return price_a < price_b;  // Lower price first for asks
    }
    return time_a < time_b;  // Earlier time first
}