#include <iostream>
#include <vector>
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
    for (auto& request : trades) {
        processRequest(move(request), orderbook);
    }

    std::cout << orderbook;
    return 0;
}
