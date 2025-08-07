#include <iostream>
#include <vector>
#include <chrono> 
#include "requests.h"
#include "orderbook.h"
#include "orders.h"
#include "basicdefs.h"
#include "testdata.h"
#include "output.h"

using namespace basicdefs;
using namespace orders;
using namespace orderbook;
using namespace requests;
using namespace testdata;
using namespace output_orderbook;

int main() {
    Orderbook orderbook;
    std::vector<std::unique_ptr<Request>> trades;

    generate_requests(trades);

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
      << tput << " req/s)\n\n";

    return 0;
}