#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <vector>
#include <memory>
#include "orderdata.h"
#include "requests.h"
#include "../core/orders.h"
#include "../core/orderbook.h"

using namespace orderdata;
using namespace requests;
using namespace orders;
using namespace orderbook;

namespace processor {

void processOrders(const std::vector<OrderData>& orders, Orderbook& orderbook) {
    for (const auto& orderData : orders) {
        if (orderData.action == "ADD") {
            if (orderData.order_type == "MARKET") {
                auto request = std::make_unique<AddRequest<MarketOrder>>(
                    orderData.side, orderData.price, orderData.quantity);
                processRequest(std::move(request), orderbook);
            } else if (orderData.order_type == "LIMIT") {
                auto request = std::make_unique<AddRequest<LimitOrder>>(
                    orderData.side, orderData.price, orderData.quantity);
                processRequest(std::move(request), orderbook);
            }
        } else if (orderData.action == "CANCEL") {
            if (orderData.order_type == "MARKET") {
                auto request = std::make_unique<CancelRequest<MarketOrder>>(
                    orderData.side, orderData.order_id);
                processRequest(std::move(request), orderbook);
            } else if (orderData.order_type == "LIMIT") {
                auto request = std::make_unique<CancelRequest<LimitOrder>>(
                    orderData.side, orderData.order_id);
                processRequest(std::move(request), orderbook);
            }
        }
    }
}

}

#endif