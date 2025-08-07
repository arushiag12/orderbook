#include <iostream>
#include <vector>
#include <memory>
#include "csvparser.h"
#include "requests.h"

using namespace csvparser;
using namespace requests;

int main() {
    std::vector<std::unique_ptr<Request>> orders;
    
    std::cout << "Loading orders from orders.csv..." << std::endl;
    loadOrdersFromCSV("orders.csv", orders);
    
    std::cout << "Successfully loaded " << orders.size() << " orders:" << std::endl;
    
    // Print summary of loaded orders
    int addCount = 0, cancelCount = 0;
    for (size_t i = 0; i < orders.size(); ++i) {
        // Since we can't easily inspect the order type without RTTI, 
        // we'll just count them
        if (orders[i]) {
            std::cout << "Order " << (i+1) << " loaded successfully" << std::endl;
            addCount++; // Simplified counting
        }
    }
    
    std::cout << "\nTotal orders loaded: " << orders.size() << std::endl;
    return 0;
}