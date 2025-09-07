#ifndef REQUEST_API_H
#define REQUEST_API_H

#include "order.h"
#include <optional>
#include <atomic>
#include <future>
#include <variant>

using ReqId = uint64_t;

struct NewOrderRequest {
    ReqId request_id{};
    Symb symbol;
    std::string order_type;
    NewOrderParams params;

    NewOrderRequest(Symb sym, std::string type, NewOrderParams p)
        : symbol(std::move(sym)), order_type(std::move(type)), params(std::move(p)) {}

};

struct CancelOrderRequest {
    ReqId request_id{};
    Symb symbol;
    OrdId order_id;
    
    CancelOrderRequest(Symb sym, OrdId id)
        : symbol(std::move(sym)), order_id(id) {}
};

struct ModifyOrderRequest {
    ReqId request_id{};
    Symb symbol;
    OrdId order_id;
    Px new_price;
    Qty new_quantity;
    
    ModifyOrderRequest(Symb sym, OrdId id, Px px, Qty qty)
        : symbol(std::move(sym)), order_id(id), new_price(px), new_quantity(qty) {}
};

using TradingRequest = std::variant<NewOrderRequest, CancelOrderRequest, ModifyOrderRequest>;

inline OrdId generate_order_id() {
    static std::atomic<OrdId> counter{1000};
    return ++counter;
}

#endif // REQUEST_API_H