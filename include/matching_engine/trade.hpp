#pragma once

#include <string>
#include <chrono>

namespace matching_engine {

struct Trade {
    using TradeId = uint64_t;
    using Price = double;
    using Quantity = uint64_t;
    using Timestamp = std::chrono::high_resolution_clock::time_point; 

    TradeId trade_id;
    std::string symbol;
    Price price;
    Quantity quantity;
    TradeId buy_order_id;
    TradeId sell_order_id;
    Timestamp timestamp;

    //constructor
    Trade(TradeId id, const std::string& sym, Price p, Quantity q, TradeId buy_id, TradeId sell_id)
        : trade_id(id), symbol(sym), price(p), quantity(q), buy_order_id(buy_id), sell_order_id(sell_id),
          timestamp(std::chrono::high_resolution_clock::now()) {} 

    std::string toString() const;
};

} // namespace matching_engine 