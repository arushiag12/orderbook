#include <iostream>
#include <vector>
#include <chrono> 
#include "requests.h"
#include "orderbook.h"
#include "orders.h"
#include "basicdefs.h"
#include "csvparser_simple.h"
#include "output.h"
#include "logger.h"

using namespace basicdefs;
using namespace orders;
using namespace orderbook;
using namespace requests;
using namespace csvparser_simple;
using namespace output_orderbook;
using namespace logger;

int main(int argc, char* argv[]) {
    // Initialize logging system
    initializeLogger("transactions.log", "errors.log");
    
    Orderbook orderbook;
    std::vector<std::unique_ptr<Request>> trades;

    // Default to orders.csv if no filename provided
    std::string filename = (argc > 1) ? argv[1] : "orders.csv";
    
    std::cout << "=== Orderbook Trading System ===" << std::endl;
    std::cout << "Loading orders from: " << filename << std::endl;
    std::cout << "Logs will be written to: transactions.log and errors.log\n" << std::endl;
    
    loadOrdersFromCSV(filename, trades);
    
    if (trades.empty()) {
        std::cout << "No orders loaded. Exiting." << std::endl;
        shutdownLogger();
        return 1;
    }
    
    std::cout << "Loaded " << trades.size() << " orders successfully.\n" << std::endl;

    // timestamp before processing
    auto t0 = std::chrono::high_resolution_clock::now();

    for (auto& request : trades) {
        processRequest(move(request), orderbook);
    }

    // timestamp after processing
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = t1 - t0;

    // compute and print throughput
    double secs = diff.count();
    double tput = trades.size() / secs;

    std::cout << orderbook;
    std::cout 
      << "\nProcessed " << trades.size() 
      << " requests in " << secs << " s  (" 
      << tput << " req/s)\n" << std::endl;
      
    std::cout << "Check transactions.log for detailed transaction history." << std::endl;
    std::cout << "Check errors.log for any errors encountered during processing." << std::endl;

    // Shutdown logging system
    shutdownLogger();
    
    return 0;
}
