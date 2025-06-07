#include "matching_engine/order.hpp"
#include <sstream>
#include <iomanip>

namespace matching_engine {

// Constructor for limit order
Order::Order(OrderId id, const std::string& symbol, OrderSide side, OrderType type, Price price, Quantity quantity)
    : id_(id)
    , symbol_(symbol)
    , side_(side)
    , type_(type)
    , price_(price)
    , quantity_(quantity)
    , remaining_quantity_(quantity)
    , timestamp_(std::chrono::high_resolution_clock::now()) {
        
        // Validating orders by checking the order ID, symbol, quantity and price based on the order type
        // Throwing an exception if the order is invalid

        // Validate order ID
        if (id == INVALID_ORDER_ID) {
            throw std::invalid_argument("Order ID cannot be zero (INVALID_ORDER_ID)");
        }
        
        // Validate symbol
        if (symbol.empty()) {
            throw std::invalid_argument("Symbol cannot be empty");
        }
        
        // Validate quantity
        if (!isValidQuantity(quantity)) {
            throw std::invalid_argument("Quantity must be between " + 
                std::to_string(MIN_QUANTITY) + " and " + std::to_string(MAX_QUANTITY));
        }
        
        // Validate price based on order type
        if (type == OrderType::MARKET) {
            if (price != MARKET_PRICE) {
                throw std::invalid_argument("Market orders must have price = 0 (MARKET_PRICE)");
            }
        } else if (type == OrderType::LIMIT) {
            if (!isValidPrice(price)) {
                throw std::invalid_argument("Limit order price must be between " + 
                    std::to_string(MIN_PRICE) + " and " + std::to_string(MAX_PRICE));
            }
        } else {
            throw std::invalid_argument("Invalid order type");
        }
    }

// Constructor for market order
Order::Order(OrderId id, const std::string& symbol, OrderSide side, Quantity quantity)
    : Order(id, symbol, side, OrderType::MARKET, MARKET_PRICE, quantity) {}

//Methods

Quantity Order::fill(Quantity fill_quantity) {
    if (fill_quantity > remaining_quantity_) {
        throw std::invalid_argument("Fill quantity exceeds remaining quantity");
    }
    remaining_quantity_ -= fill_quantity;
    return fill_quantity;
}

bool Order::canMatchWith(const Order& other) const noexcept {
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

bool Order::hasHigherPriorityThan(const Order& other) const noexcept {
    // Different symbols shouldn't be compared
    if (symbol_ != other.symbol_) return false;
    
    // Different sides shouldn't be compared  
    if (side_ != other.side_) return false;
    
    if (isBuyOrder()) {
        // Buy orders: higher price wins, then earlier time
        if (price_ != other.price_) {
            return price_ > other.price_;
        }
        return timestamp_ < other.timestamp_; // Earlier time wins
    } else {
        // Sell orders: lower price wins, then earlier time
        if (price_ != other.price_) {
            return price_ < other.price_;
        }
        return timestamp_ < other.timestamp_; // Earlier time wins
    }
}

std::string Order::toString() const {
    std::ostringstream oss;
    oss << "Order{"
        << "id=" << id_
        << ", symbol=" << symbol_
        << ", side=" << matching_engine::toString(side_)
        << ", type=" << matching_engine::toString(type_)
        << ", price=" << std::fixed << std::setprecision(2) << price_
        << ", qty=" << quantity_
        << ", remaining=" << remaining_quantity_
        << "}";
    return oss.str(); //returning the string representation of the order
}

// =============================================================================
// Global Operator Implementations
// =============================================================================

std::ostream& operator<<(std::ostream& os, const Order& order) { //overloading the << operator to print the order, which means that we can use the << operator to print the order
    os << order.toString();
    return os;
}

bool operator==(const Order& lhs, const Order& rhs) noexcept { //overloading the == operator to compare two orders
    return lhs.getId() == rhs.getId();
}

bool operator!=(const Order& lhs, const Order& rhs) noexcept { //overloading the != operator to compare two orders
    return !(lhs == rhs);
}

} // namespace matching_engine