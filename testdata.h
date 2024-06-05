#ifndef TESTDATA_H
#   define TESTDATA_H

#include <vector>
#include "requests.h"
#include "orders.h"

using orders::MarketOrder;
using orders::LimitOrder;
using requests::AddRequest;
using requests::CancelRequest;

namespace testdata {

void generate_requests(auto& trades) {
    trades.push_back(std::make_unique<AddRequest<MarketOrder>>(Side::BUY, 100, 10));
    trades.push_back(std::make_unique<AddRequest<LimitOrder>>(Side::BUY, 200, 20));
    trades.push_back(std::make_unique<AddRequest<MarketOrder>>(Side::BUY, 300, 30));
    trades.push_back(std::make_unique<AddRequest<MarketOrder>>(Side::SELL, 300, 30));
    trades.push_back(std::make_unique<AddRequest<LimitOrder>>(Side::SELL, 200, 20));
    trades.push_back(std::make_unique<AddRequest<LimitOrder>>(Side::SELL, 100, 10));
    trades.push_back(std::make_unique<AddRequest<MarketOrder>>(Side::SELL, 50, 5));
    trades.push_back(std::make_unique<AddRequest<MarketOrder>>(Side::BUY, 50, 5));
    trades.push_back(std::make_unique<CancelRequest<LimitOrder>>(Side::BUY, 2));
    trades.push_back(std::make_unique<AddRequest<LimitOrder>>(Side::BUY, 200, 10));
    trades.push_back(std::make_unique<AddRequest<LimitOrder>>(Side::BUY, 300, 10));
    trades.push_back(std::make_unique<AddRequest<MarketOrder>>(Side::SELL, 100, 10));
    trades.push_back(std::make_unique<CancelRequest<LimitOrder>>(Side::SELL, 3));
}

}
#endif