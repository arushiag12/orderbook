#include <exchange.h>
#include <iostream>

Exchange::AssetContext::AssetContext(const std::string& symbol, ThreadPool& threadPool) 
    : symbol_(symbol),
      strand_(std::make_unique<Strand>(threadPool)),
      orderBook_(std::make_unique<OrderBook>(symbol)) {}

Exchange::Exchange(const std::vector<std::string>& symbols, size_t numWorkerThreads) 
    : threadPool_(numWorkerThreads),
      logger_(std::make_unique<Logger>("logs_internal/")),
      matchingEngine_(
          // OrderLogger callback
          [this](const OrderLog& orderLog) { 
              logger_->logOrderEvent(orderLog); 
          },
          // TradeLogger callback  
          [this](const TradeLog& tradeLog) { 
              logger_->logTradeEvent(tradeLog); 
          }
      ) {
    
    // create AssetContexts for all specified symbols
    for (const auto& symbol : symbols) {
        assets_[symbol] = std::make_unique<AssetContext>(symbol, threadPool_);
    }
}

Exchange::~Exchange() {
    shutdown();
}

RequestOutcome Exchange::processRequest(TradingRequest&& req) {
    auto requestId = std::visit([](const auto& r) { return r.request_id; }, req);
    auto symbol = std::visit([](const auto& r) { return r.symbol; }, req);
    AssetContext& ac = getAssetContext(symbol)->get();
    RequestOutcome outcome;

    ac.strand_->post(
        [this, &ac, &outcome, req = std::move(req)]() mutable {
            outcome = matchingEngine_.process_request(*ac.orderBook_, std::move(req));
            onRequestProcessed(outcome);
        }
    );
    return outcome;
}

std::optional<Exchange::AssetRef> Exchange::getAssetContext(std::string_view symbol) {
    auto it = assets_.find(std::string(symbol));
    if (it == assets_.end()) return std::nullopt;
    return std::ref(*it->second);
}

void Exchange::shutdown() {
    threadPool_.shutdown();
    if (logger_) {
        logger_->shutdown();
    }
}

void Exchange::onRequestProcessed(const RequestOutcome& outcome) {
    // Log the outcome using the logger
    logger_->logRequestOutcome(outcome);
}