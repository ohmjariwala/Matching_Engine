#pragma once // prevent multiple inclusions

#include "types.hpp"
#include <string> // for std::string
#include <chrono> // for time-related operations
#include <ostream> // for output stream
#include <stdexcept>  // Added missing include

namespace matching_engine {

/**
 * @brief Represents a trading order in the matching engine
 * 
 * An order contains the following information for matching:
 * - Unique Identifier and symbol
 * - Side (buy or sell) and type (market or limit)
 * - Price and quantity
 * - Timestamp for price-time priority
 * 
 * Market orders have price = 0 and are matched immediately at the best available price.
 * Limit orders specify a max (buy) or min (sell) price and are matched when the market price reaches or exceeds the limit price.
 */
class Order {
    private:
        OrderId id_;
        std::string symbol_;
        OrderSide side_;
        OrderType type_;
        Price price_;
        Quantity quantity_;
        Quantity remaining_quantity_;
        std::chrono::high_resolution_clock::time_point timestamp_;


    public:
    /**
     * @brief Construct a new Order object
     * 
     * @param id Unique identifier for the order
     * @param symbol Symbol of the instrument being traded
     * @param side Buy or sell side
     * @param type Market or limit order
     * @param price Order price (0 for market orders)
     * @param quantity Order quantity
     */
    Order(OrderId id, const std::string& symbol, OrderSide side, OrderType type, Price price, Quantity quantity);

    /**
     * @brief Construct a market order (price = 0)
     * 
     * @param id Unique identifier for the order
     * @param symbol Symbol of the instrument being traded
     * @param side Buy or sell side
     * @param quantity Quantity of the order
     */
    Order(OrderId id, const std::string& symbol, OrderSide side, Quantity quantity); //same as above but price is 0

    //Getters
    OrderId getId() const noexcept { return id_; }
    const std::string& getSymbol() const noexcept { return symbol_; }
    OrderSide getSide() const noexcept { return side_; }
    OrderType getType() const noexcept { return type_; }
    Price getPrice() const noexcept { return price_; }
    Quantity getQuantity() const noexcept { return quantity_; }
    Quantity getRemainingQuantity() const noexcept { return remaining_quantity_; }

    /**
     * @brief Get the order timestamp for FIFO ordering
     * @return High-resolution timestamp when order was created
     */
    std::chrono::high_resolution_clock::time_point getTimestamp() const noexcept { return timestamp_; }


    //Utility methods
    /**
     * @brief Check if market order
     */
    bool isMarketOrder() const noexcept { return type_ == OrderType::MARKET; }

    /**
     * @brief Check if limit order
     */
    bool isLimitOrder() const noexcept { return type_ == OrderType::LIMIT; }

    /**
     * @brief Check if buy order
     */
    bool isBuyOrder() const noexcept { return side_ == OrderSide::BUY; }

    /**
     * @brief Check if sell order
     */
    bool isSellOrder() const noexcept { return side_ == OrderSide::SELL; }

    /**
     * @brief Check if order is completely filled
     * @return true if remaining quantity is 0
     */
    bool isFullyFilled() const noexcept { return remaining_quantity_ == 0; }

    /**
     * @brief Check if order is partially filled
     * @return true if some but not all quantity has been filled
     */
    bool isPartiallyFilled() const noexcept { return remaining_quantity_ > 0 && remaining_quantity_ < quantity_; }

    /**
     * @brief Fill part of the order
     * 
     * @param fill_quantity Amount to fill
     * @return Actual quantity filled (may be less than requested)
     * @throws std::invalid_argument if fill_quantity > remaining_quantity_
     */
    Quantity fill(Quantity fill_quantity) {
        if (fill_quantity > remaining_quantity_) {
            throw std::invalid_argument("Fill quantity exceeds remaining quantity");
        }
        remaining_quantity_ -= fill_quantity;
        return fill_quantity;
    }
        
    /**
     * @brief Check if this order can match with another order
     * 
     * Order will match if:
     * - They have opposite sides (buy vs sell) and same symbol
     * - Price conditions are met (buy price >= sell price for limit orders)
     * 
     * @param other The other order to check against for matching
     * @return True if orders can match, false otherwise
     */
    bool canMatchWith(const Order& other) const noexcept {
        // Must be same symbol
        if (symbol_ != other.symbol_) return false;
        
        // Must be opposite sides
        if (side_ == other.side_) return false;
        
        // Market orders can always match (if symbol and side are compatible)
        if (isMarketOrder() || other.isMarketOrder()) return true;
        
        // For limit orders, check price compatibility
        if (isBuyOrder()) {
            // This is a buy order, other is a sell order
            // Buy price must be >= sell price
            return price_ >= other.price_;
        } else {
            // This is a sell order, other is a buy order  
            // Sell price must be <= buy price
            return price_ <= other.price_;
        }
    }

    /**
     * @brief Compare orders for priority in order book (price-time priority)
     * 
     * For buy orders: higher price has priority, then earlier time
     * For sell orders: lower price has priority, then earlier time
     * 
     * @param other Order to compare with
     * @return true if this order has higher priority
     */
    bool hasHigherPriorityThan(const Order& other) const noexcept {
        if (symbol_ != other.symbol_) return false; //must be same symbol
        if (side_ != other.side_) return false; //must be opposite sides

        if (isBuyOrder()) {
            if (price_ != other.price_) {
                return price_ > other.price_;
            }
            return timestamp_ < other.timestamp_;
        } else { //sell order
            if (price_ != other.price_) {
                return price_ < other.price_;
            }
            return timestamp_ < other.timestamp_;
        }
    }

    // String representation for debugging
    std::string toString() const;
};

// Stream operator for easy printing
std::ostream& operator<<(std::ostream& os, const Order& order);

// Comparison operators
bool operator==(const Order& lhs, const Order& rhs) noexcept;
bool operator!=(const Order& lhs, const Order& rhs) noexcept;

} // namespace matching_engine

