#include <iostream>
#include <vector>
#include <chrono> 
#include "io/requests.h"
#include "core/orderbook.h"
#include "core/orders.h"
#include "core/basicdefs.h"
#include "io/orderdata.h"
#include "io/csvparser.h"
#include "io/processor.h"
#include "io/output.h"
#include "utils/logger.h"

using namespace basicdefs;
using namespace orders;
using namespace orderbook;
using namespace requests;
using namespace orderdata;
using namespace csvparser;
using namespace processor;
using namespace output_orderbook;
using namespace logger;

int main(int argc, char* argv[]) {
    // Initialize logging system
    initializeLogger("logs/transactions.log", "logs/errors.log");
    
    Orderbook orderbook;
    std::vector<OrderData> orders;

    // Default to data/orders.csv if no filename provided
    std::string filename = (argc > 1) ? argv[1] : "data/orders.csv";
    
    std::cout << "=== Orderbook Trading System ===" << std::endl;
    std::cout << "Loading orders from: " << filename << std::endl;
    // std::cout << "Logs will be written to: logs/transactions.log and logs/errors.log\n" << std::endl;
    
    loadOrdersFromCSV(filename, orders);
    
    if (orders.empty()) {
        std::cout << "No orders loaded. Exiting." << std::endl;
        shutdownLogger();
        return 1;
    }
    
    // std::cout << "Loaded " << orders.size() << " orders successfully.\n" << std::endl;

    // timestamp before processing
    auto t0 = std::chrono::high_resolution_clock::now();

    processOrders(orders, orderbook);

    // timestamp after processing
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = t1 - t0;

    // compute and print throughput
    double secs = diff.count();
    double tput = orders.size() / secs;

    // std::cout << orderbook;
    std::cout 
      << "\nProcessed " << orders.size() 
      << " requests in " << secs << " s  (" 
      << tput << " req/s)\n" << std::endl;
      
    std::cout << "Check logs/transactions.log for detailed transaction history." << std::endl;
    std::cout << "Check logs/errors.log for any errors encountered during processing." << std::endl;

    // Shutdown logging system
    shutdownLogger();
    
    return 0;
}
