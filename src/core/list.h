#ifndef LIST_H
#   define LIST_H

#include <memory>
#include "basicdefs.h"
#include "orders.h"

using orders::Order;
namespace my_list{

inline void addItem(std::unique_ptr<Order>& order, std::list<std::unique_ptr<Order>>& l) {
    auto it = l.begin();
    if (it == l.end()) {
        l.push_back(std::move(order));
    } else {
        while ( (it != l.end()) && ((**it) > *order)) {
            ++it;
        } 
        l.insert(it, std::move(order));
    }
}

inline void deleteItem(OrderId id, std::list<std::unique_ptr<Order>>& l) {
    auto it = l.begin();
    while (it != l.end()) {
        if ((*it)->id == id) {
            l.erase(it);
            break;
        }
        ++it;
    }
}

}
#endif
