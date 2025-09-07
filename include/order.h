#ifndef ORDER_H
#define ORDER_H

#include <chrono>
#include <string>
#include <string_view>
#include <variant>
#include <tuple>
#include <array>
#include <optional>
#include <concepts>
#include <cstdint>

// domain types
using OrdId   = uint64_t;
using Px     = double;
using Qty  = uint32_t;
using Symb    = std::string;
using ClientId  = std::string;
using Timestamp = std::chrono::steady_clock::time_point;

enum class Side { BUY, SELL };
enum class OrdState { PENDING, ACTIVE, PARTIALLY_FILLED, FILLED, CANCELLED, REJECTED };

// common fields
struct OrderMeta {
	OrdId    order_id;
	ClientId client_id;
	Side     side;
	Px      price;
	Qty      original_quantity;
	Qty      remaining_quantity;
	OrdState state;
	Timestamp   timestamp;

	OrderMeta(OrdId oid, const ClientId& cid, Side s, Px p, Qty qty)
		: order_id(oid), client_id(cid), side(s), price(p),
			original_quantity(qty), remaining_quantity(qty),
			state(OrdState::PENDING), timestamp(std::chrono::steady_clock::now()) {}
};

// concrete orders
struct MarketOrder{
	OrderMeta meta;
	inline static constexpr std::string_view kName = "MARKET";

	MarketOrder(OrdId id, const ClientId& c, Side s, Qty q)
		: meta{id, c, s, 0.0, q} {}
	static MarketOrder create(struct NewOrderParams const& p) {
		return MarketOrder(p.id, p.client, p.side, p.qty);
	}
};

struct LimitOrder {
	OrderMeta meta;
	inline static constexpr std::string_view kName = "LIMIT";

	LimitOrder(OrdId id, const ClientId& c, Side s, Px px, Qty q)
		: meta{id, c, s, px, q} {}
	static LimitOrder create(struct NewOrderParams const& p) {
		return LimitOrder(p.id, p.client, p.side, p.price.value_or(0.0), p.qty);
	}
};

// unified construction params
struct NewOrderParams {
	OrdId   id{};
	ClientId  client;
	Side      side;
	std::optional<Px> price; // ignored by market
	Qty  qty;
};

// concept: each type must expose kName and static create(NewOrderParams)->T
template<class T>
concept OrderTypeRequirement = requires (const NewOrderParams& p) {
	{ T::kName } -> std::convertible_to<std::string_view>;
	{ T::create(p) } -> std::same_as<T>;
};

// list types once
template<OrderTypeRequirement... Ts>
struct Types {};
using OrderTypes = Types<MarketOrder, LimitOrder>;

// derive variant/tuple
template<class> struct to_variant;
template<OrderTypeRequirement... Ts>
struct to_variant<Types<Ts...>> { using type = std::variant<Ts...>; };

template<class> struct to_tuple;
template<OrderTypeRequirement... Ts>
struct to_tuple<Types<Ts...>>   { using type = std::tuple<Ts...>; };

using Order = typename to_variant<OrderTypes>::type;
using OrderTypesTuple = typename to_tuple<OrderTypes>::type;

// names array derived from types
template<OrderTypeRequirement... Ts>
consteval auto names_array(Types<Ts...>) {
    return std::array<std::string_view, sizeof...(Ts)>{ Ts::kName... };
}
inline constexpr auto kOrderNames = names_array(OrderTypes{});  // {"MARKET","LIMIT"}

// dispatch by name → call T::create and return Order
template<class, class F>
inline bool dispatch_by_name(std::string_view, F&&);

template<OrderTypeRequirement... Ts, class F>
inline bool dispatch_by_name(Types<Ts...>, std::string_view name, F&& f) {
	bool handled = false;
	((name == Ts::kName ? (f.template operator()<Ts>(), handled = true) : void()), ...);
	return handled;
}

struct Maker {
	const NewOrderParams& p;
	Order out;
	Maker(const NewOrderParams& params) : p(params), out(MarketOrder(0, "", Side::BUY, 0)) {}
	template<class T> void operator()() { out = T::create(p); }
};

// factory function for creating an order
inline std::unique_ptr<Order>
create_order(std::string_view kind, const NewOrderParams& p) {
	Maker m{p};
	dispatch_by_name(OrderTypes{}, kind, m);
	return std::make_unique<Order>(std::move(m.out));
}

#endif
