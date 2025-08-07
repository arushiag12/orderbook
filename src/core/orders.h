#ifndef ORDERS_H
#   define ORDERS_H

#include<tuple>
#include<chrono>
#include<atomic>
#include "basicdefs.h"

using basicdefs::Price;
using basicdefs::Quantity;
using basicdefs::Side;
using basicdefs::OrderId;
namespace orders {

struct Order {
    Order() = delete;
    Order(Side side, Price price, Quantity quantity) 
        : side(side), price(price), quantity(quantity), 
          id(getNextOrderId()), timestamp(std::chrono::steady_clock::now()) {}
    virtual ~Order() = default;

    Side side;
    Price price;
    Quantity quantity;
    OrderId id;
    std::chrono::steady_clock::time_point timestamp;

    static OrderId getNextOrderId() {
        static std::atomic<OrderId> orderIdCounter{1};
        return orderIdCounter.fetch_add(1);
    }

    bool operator<(const Order& other) const {
        // For buy orders: higher price has priority, if same price then earlier time
        // For sell orders: lower price has priority, if same price then earlier time
        if (side == Side::BUY) {
            if (price != other.price) {
                return price < other.price; // Lower price has lower priority for buy
            }
        } else {
            if (price != other.price) {
                return price > other.price; // Higher price has lower priority for sell
            }
        }
        return timestamp > other.timestamp; // Later timestamp has lower priority
    }

    bool operator>(const Order& other) const {
        return other < *this;
    }
};

struct MarketOrder : public Order { using Order::Order; };
struct LimitOrder : public Order {using Order::Order; };

using OrderTypes = std::tuple<MarketOrder, LimitOrder>;



template<typename T, typename S> struct CheckValidOrderType;

template<typename T, typename H, typename... Ts>
struct CheckValidOrderType<T, std::tuple<H, Ts...>> {
    static bool constexpr value = std::is_same_v<T, H> || CheckValidOrderType<T, std::tuple<Ts...>>::value;
};

template <typename T>
struct CheckValidOrderType<T, std::tuple<>> {
    static bool constexpr value = false;
};

template <typename T, typename S>
bool constexpr CheckValidOrderType_v = CheckValidOrderType<T, S>::value;

}
#endif
