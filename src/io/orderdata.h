#ifndef ORDERDATA_H
#define ORDERDATA_H

#include <string>
#include "basicdefs.h"

using namespace basicdefs;

namespace orderdata {

struct OrderData {
    std::string action;      // "ADD", "CANCEL"
    std::string order_type;  // "MARKET", "LIMIT"
    Side side;              // BUY, SELL
    Price price;
    Quantity quantity;
    OrderId order_id;       // For cancellations
    
    OrderData(const std::string& act, const std::string& type, Side s, 
              Price p, Quantity q, OrderId id = 0)
        : action(act), order_type(type), side(s), price(p), quantity(q), order_id(id) {}
};

}

#endif