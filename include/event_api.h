#ifndef EVENT_API_H
#define EVENT_API_H

#include "order.h"
#include "request_api.h"
#include <ostream>
#include <iomanip>
#include <functional>

struct Fill {
  Symb symbol;
  OrdId taker_id;       // the incoming (active) order
  OrdId maker_id;       // the resting order hit
  Px price;
  Qty qty;
  bool taker_is_buy;   // true if taker side was BUY
  Timestamp ts;             // match timestamp
  uint64_t  match_seq;      // per-asset sequence (optional but great for logs)
};

enum class RequestStatus { OK, REJECTED, NOOP };
enum class RejectReason {
  NONE, UNKNOWN_SYMBOL, UNKNOWN_ORDER, INVALID_PRICE, INVALID_QUANTITY,
  NOT_MODIFIABLE, BOOK_CLOSED
};

struct RequestOutcome {
  ReqId            request_id{};
  RequestStatus        status{RequestStatus::OK};
  RejectReason         reason{RejectReason::NONE}; // valid when REJECTED/NOOP
  std::string          message;                    // human-friendly summary (optional)
  std::vector<Fill>    fills;                      // taker-view fills produced by *this* request only
  // Optional convenience:
  Qty             taker_filled_qty{};         // sum of fills
  Qty             taker_remaining_qty{};      // after processing (for NEW/MODIFY)
};

struct RequestLog {
  std::string symbol;
  ReqId   request_id{};
  RequestStatus status{};
  RejectReason  reason{RejectReason::NONE};
  std::string   message;       // same as outcome.message if you want
  Timestamp     ts{};
};

enum class OrderEventType {
  NEW_ACCEPTED, REPLACED, CANCELED, EXPIRED, REJECTED,
  PARTIALLY_FILLED, FILLED
};

struct OrderLog {
  std::string symbol;
  uint64_t    seq{};           // per-asset sequence (monotonic)
  Timestamp   ts{};

  OrderEventType type{};
  OrdId    order_id{};
  Side       side{};
  Px      price{};          // current/resting price after this event
  Qty   remaining_qty{};  // after this event
  RejectReason reason{RejectReason::NONE}; // for REJECTED/EXPIRED
};

struct TradeLog {
  std::string symbol;
  uint64_t    seq{};           // per-asset sequence
  Timestamp   ts{};

  Fill        fill;            // embed your Fill directly
};

using OrderLogger = std::function<void(const OrderLog&)>;
using TradeLogger = std::function<void(const TradeLog&)>;

inline std::ostream& operator<<(std::ostream& os, const Fill& fill) {
    return os << fill.symbol << "," << fill.taker_id << "," << fill.maker_id << "," 
              << std::fixed << std::setprecision(2) << fill.price << "," 
              << fill.qty << "," << (fill.taker_is_buy ? "BUY" : "SELL") << "," 
              << fill.match_seq;
}

inline std::ostream& operator<<(std::ostream& os, OrderEventType type) {
    switch (type) {
        case OrderEventType::NEW_ACCEPTED: return os << "NEW_ACCEPTED";
        case OrderEventType::REPLACED: return os << "REPLACED";
        case OrderEventType::CANCELED: return os << "CANCELED";
        case OrderEventType::EXPIRED: return os << "EXPIRED";
        case OrderEventType::REJECTED: return os << "REJECTED";
        case OrderEventType::PARTIALLY_FILLED: return os << "PARTIALLY_FILLED";
        case OrderEventType::FILLED: return os << "FILLED";
        default: return os << "UNKNOWN";
    }
}

inline std::ostream& operator<<(std::ostream& os, Side side) {
    return os << (side == Side::BUY ? "BUY" : "SELL");
}

inline std::ostream& operator<<(std::ostream& os, RequestStatus status) {
    switch (status) {
        case RequestStatus::OK: return os << "OK";
        case RequestStatus::REJECTED: return os << "REJECTED";
        case RequestStatus::NOOP: return os << "NOOP";
        default: return os << "UNKNOWN";
    }
}

inline std::ostream& operator<<(std::ostream& os, RejectReason reason) {
    switch (reason) {
        case RejectReason::NONE: return os << "NONE";
        case RejectReason::UNKNOWN_SYMBOL: return os << "UNKNOWN_SYMBOL";
        case RejectReason::UNKNOWN_ORDER: return os << "UNKNOWN_ORDER";
        case RejectReason::INVALID_PRICE: return os << "INVALID_PRICE";
        case RejectReason::INVALID_QUANTITY: return os << "INVALID_QUANTITY";
        case RejectReason::NOT_MODIFIABLE: return os << "NOT_MODIFIABLE";
        case RejectReason::BOOK_CLOSED: return os << "BOOK_CLOSED";
        default: return os << "UNKNOWN";
    }
}

inline std::ostream& operator<<(std::ostream& os, const OrderLog& orderLog) {
    auto time_t = orderLog.ts.time_since_epoch().count() / 1000000000LL;
    
    return os << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << ","
              << orderLog.symbol << "," << orderLog.seq << "," << orderLog.type << ","
              << orderLog.order_id << "," << orderLog.side << ","
              << std::fixed << std::setprecision(2) << orderLog.price << ","
              << orderLog.remaining_qty;
}

inline std::ostream& operator<<(std::ostream& os, const TradeLog& tradeLog) {
    auto time_t = tradeLog.ts.time_since_epoch().count() / 1000000000LL;
    return os << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << ","
              << tradeLog.symbol << "," << tradeLog.seq << "," << tradeLog.fill;
}

inline std::ostream& operator<<(std::ostream& os, const RequestOutcome& outcome) {
    os << outcome.request_id << "," << outcome.status << "," << outcome.reason << ","
       << "\"" << outcome.message << "\"," << outcome.taker_filled_qty << ","
       << outcome.taker_remaining_qty << "," << outcome.fills.size();
    
    // Add fills if any
    for (const auto& fill : outcome.fills) {
        os << ",[" << fill << "]";
    }
    
    return os;
}

#endif