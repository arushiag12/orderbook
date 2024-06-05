#ifndef ORDERS_H
#   define ORDERS_H

#include<tuple>
#include "basicdefs.h"

using basicdefs::Price;
using basicdefs::Quantity;
using basicdefs::Side;
using basicdefs::OrderId;
namespace orders {

struct Order {
    Order() = delete;
    Order(Side side, Price price, Quantity quantity) 
        : side(side), price(price), quantity(quantity), id(currOrderIdCount++) {}
    virtual ~Order() = default;

    static OrderId currOrderIdCount;

    Side side;
    Price price;
    Quantity quantity;
    OrderId id;

    bool operator<(const Order& other) const {
        if (price == other.price) {
            return quantity < other.quantity;
        }
        return price > other.price;
    }

    bool operator>(const Order& other) const {
        if (price == other.price) {
            return quantity > other.quantity;
        }
        return price < other.price;
    }
};
OrderId Order::currOrderIdCount = 0;

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
