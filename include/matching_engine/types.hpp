#pragma once // prevent multiple inclusions

#include <cstdint>
#include <string>


namespace matching_engine {


// Basic Type Aliases

/**
 * @brief Unique identifier for orders
 * 
 * Using 64-bit unsigned integer to support very large numbers of orders without risk of overflow in high-frequency trading scenarios.
 */
using OrderId = uint64_t;

/**
 * @brief Price representation
 * 
 * Using double for simplicity. In production systems, you might want to use fixed-point arithmetic or a decimal library to avoid floating-point precision issues.
 */
using Price = double;

/**
 * @brief Quantity representation
 * 
 * Using 64-bit unsigned integer to handle large quantities without overflow.
 */
using Quantity = uint64_t;

/**
 * @brief Trade identifier
 */
using TradeId = uint64_t;

/**
 * @brief Symbol type for trading instruments
 */
using Symbol = std::string;



// Enumerations

/**
 * @brief Order side enumeration
 * 
 * Represents whether an order is a buy or sell order.
 * Using uint8_t for memory efficiency in high-frequency scenarios.
 */
enum class OrderSide : uint8_t {
    BUY = 0,   ///< Buy order (bid)
    SELL = 1   ///< Sell order (ask/offer)
};


/**
 * @brief Order type enumeration
 * 
 * Represents different types of orders supported by the matching engine.
 */
enum class OrderType : uint8_t {
    MARKET = 0,  
    LIMIT = 1   
};

/**
 * @brief Order status enumeration
 * 
 * Tracks the current state of an order in the system.
 */
enum class OrderStatus : uint8_t {
    PENDING = 0,     ///< Order received but not yet processed
    ACTIVE = 1,      ///< Order is active in the order book
    PARTIALLY_FILLED = 2,  ///< Order is partially executed
    FULLY_FILLED = 3,      ///< Order is fully executed
    CANCELLED = 4,   
    REJECTED = 5 
};


/**
 * @brief Trade side from perspective of the aggressive order
 * 
 * Indicates which side initiated the trade (took liquidity).
 */
enum class TradeSide : uint8_t {
    BUY = 0,   ///< Trade initiated by a buy order
    SELL = 1   ///< Trade initiated by a sell order
};


// Constants


/**
 * @brief Invalid/null order ID constant
 */
constexpr OrderId INVALID_ORDER_ID = 0;

/**
 * @brief Invalid/null trade ID constant
 */
constexpr TradeId INVALID_TRADE_ID = 0;

/**
 * @brief Minimum valid price (prevents negative or zero prices for limit orders)
 */
constexpr Price MIN_PRICE = 0.01;

/**
 * @brief Maximum valid price (prevents unreasonably high prices)
 */
constexpr Price MAX_PRICE = 1e9;

/**
 * @brief Minimum valid quantity
 */
constexpr Quantity MIN_QUANTITY = 1;

/**
 * @brief Maximum valid quantity
 */
constexpr Quantity MAX_QUANTITY = 1e9;

/**
 * @brief Price used for market orders (convention: 0 means "any price")
 */
constexpr Price MARKET_PRICE = 0.0;


// Utility Functions

/**
 * @brief Convert OrderSide enum to string
 * @param side The order side to convert
 * @return String representation of the order side
 */
constexpr const char* toString(OrderSide side) noexcept {
    switch (side) {
        case OrderSide::BUY:  return "BUY";
        case OrderSide::SELL: return "SELL";
        default:              return "UNKNOWN";
    }
}

/**
 * @brief Convert OrderType enum to string
 * @param type The order type to convert
 * @return String representation of the order type
 */
constexpr const char* toString(OrderType type) noexcept {
    switch (type) {
        case OrderType::MARKET: return "MARKET";
        case OrderType::LIMIT:  return "LIMIT";
        default:                return "UNKNOWN";
    }
}

/**
 * @brief Convert OrderStatus enum to string
 * @param status The order status to convert
 * @return String representation of the order status
 */
constexpr const char* toString(OrderStatus status) noexcept {
    switch (status) {
        case OrderStatus::PENDING:         return "PENDING";
        case OrderStatus::ACTIVE:          return "ACTIVE";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FULLY_FILLED:    return "FULLY_FILLED";
        case OrderStatus::CANCELLED:       return "CANCELLED";
        case OrderStatus::REJECTED:        return "REJECTED";
        default:                           return "UNKNOWN";
    }
}

/**
 * @brief Convert TradeSide enum to string
 * @param side The trade side to convert
 * @return String representation of the trade side
 */
constexpr const char* toString(TradeSide side) noexcept {
    switch (side) {
        case TradeSide::BUY:  return "BUY";
        case TradeSide::SELL: return "SELL";
        default:              return "UNKNOWN";
    }
}

/**
 * @brief Get opposite side for an order
 * @param side The original order side
 * @return The opposite side
 */
constexpr OrderSide getOppositeSide(OrderSide side) noexcept {
    return (side == OrderSide::BUY) ? OrderSide::SELL : OrderSide::BUY;
}

/**
 * @brief Check if a price is valid for a limit order
 * @param price The price to validate
 * @return true if price is within valid range
 */
constexpr bool isValidPrice(Price price) noexcept {
    return price >= MIN_PRICE && price <= MAX_PRICE;
}

/**
 * @brief Check if a quantity is valid
 * @param quantity The quantity to validate
 * @return true if quantity is within valid range
 */
constexpr bool isValidQuantity(Quantity quantity) noexcept {
    return quantity >= MIN_QUANTITY && quantity <= MAX_QUANTITY;
}

} // namespace matching_engine