#ifndef EXCHANGE_H
#define EXCHANGE_H

#include "order.h"
#include "request_api.h"
#include "event_api.h"
#include "thread_pool.h"
#include "strand.h"
#include "logger.h"
#include "matching_engine.h"
#include <vector>
#include <future>
#include <functional>
#include <unordered_map>
#include <memory>
#include <optional>


class Exchange {
public:
    explicit Exchange(const std::vector<Symb>& symbols, size_t num_worker_threads = std::thread::hardware_concurrency());
    ~Exchange();

    RequestOutcome processRequest(TradingRequest&& tr);
    void shutdown();

private:

    struct AssetContext{
        explicit AssetContext(const std::string& symbol, ThreadPool& threadPool);
        std::unique_ptr<Strand> strand_;
        std::unique_ptr<OrderBook> orderBook_;
        std::string symbol_;
    };
    using AssetRef = std::reference_wrapper<AssetContext>;

    std::optional<AssetRef> getAssetContext(std::string_view symbol);

    void onRequestProcessed(const RequestOutcome& outcome);

    ThreadPool threadPool_;
    std::unique_ptr<Logger> logger_;
    MatchingEngine matchingEngine_;
    std::unordered_map<std::string, std::unique_ptr<AssetContext>> assets_;
};


#endif