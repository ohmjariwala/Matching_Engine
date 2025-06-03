#pragma once

#include <matching_engine/types.hpp>
#include <matching_engine/order.hpp>
#include <map> //for buy and sell orders
#include <queue> //for FIFO order processing
#include <vector> //for trade execution results
#include <optional> //for optional values
#include <unordered_map> //for fast order lookup

namespace matching_engine {

/**
 * @brief Represents a single trade execution result
 * 
 * Generated when two orders are matched and executed.
 */
struct Trade {
    TradeId trade_id;
    OrderId buy_order_id;
    OrderId sell_order_id;
    Price execution_price;
    Quantity quantity;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    Trade(TradeId trade_id, OrderId buy_order_id, OrderId sell_order_id, Price execution_price, Quantity quantity)
        : trade_id(trade_id), buy_order_id(buy_order_id), sell_order_id(sell_order_id), 
          execution_price(execution_price), quantity(quantity),
          timestamp(std::chrono::high_resolution_clock::now()) {}
};

/**
 * @brief Order book class maintaining buy and sell orders with price-time priority
 * 
 * Implements FIFO matching within price levels:
 * - Buy orders: Higher prices have priority, then earlier time (FIFO)
 * - Sell orders: Lower prices have priority, then earlier time (FIFO)
 * - Market orders: Execute immediately against best available prices
 * 
 * Data Structure:
 * - Buy orders: std::map with descending price order (highest price first)
 * - Sell orders: std::map with ascending price order (lowest price first)
 */


class OrderBook {
    private:
        // Price level maps: Price -> Queue of orders at that price
        std::map<Price, std::queue<Order>, std::greater<Price>> bids_;        // Descending: highest price first
        std::map<Price, std::queue<Order>, std::less<Price>> asks_;     // Ascending: lowest price first
        
        // Fast order lookup for cancellations
        std::unordered_map<OrderId, std::pair<Price, OrderSide>> order_locations_; //hashmap with order id, (price, side) as key value pair
        
        // Trade ID generator
        TradeId next_trade_id_;
        
        /**
         * @brief Execute a market order against existing limit orders
         * @param market_order The market order to execute
         * @return Vector of trades
         */
        std::vector<Trade> executeMarketOrder(Order& market_order);
        
        /**
         * @brief Try to match a limit order against existing orders
         * @param limit_order The limit order to match
         * @return Vector of trades
         */
        std::vector<Trade> matchLimitOrder(Order& limit_order);
        
        /**
         * @brief Add a limit order to the appropriate price level
         * @param order The order to add to the book
         * @return void
         */ 
        void addToBook(const Order& order);
        
        /**
         * @brief Remove an order from a specific price level
         * @param price The price level to remove from
         * @param side The side (buy/sell) to remove from
         * @param order_id The order ID to remove
         * @return true if order was found and removed
         */
        bool removeFromPriceLevel(Price price, OrderSide side, OrderId order_id);
        
        /**
         * @brief Generate a new trade ID
         * @return Unique trade identifier
         */
        TradeId generateTradeId() { return ++next_trade_id_; }

    public:
        /**
         * @brief Construct a new Order Book
         */
        OrderBook() : next_trade_id_(0) {}
        
        /**
         * @brief Add an order to the order book and attempt matching
         * 
         * Process:
         * 1. If market order: Execute immediately against best available prices
         * 2. If limit order: Try to match against existing orders
         * 3. If not fully filled: Add remaining quantity to order book
         * 
         * @param order The order to add
         * @return Vector of trades generated from matching
         */
        std::vector<Trade> addOrder(Order order);
        
        /**
         * @brief Cancel an existing order
         * 
         * @param order_id The ID of the order to cancel
         * @return true if order was found and cancelled, false otherwise
         */
        bool cancelOrder(OrderId order_id); 
        
        /**
         * @brief Get the best bid price (highest buy price)
         * @return Best bid price, or std::nullopt if no bids exist
         */
        std::optional<Price> getBestBid() const; //std::optional is a container that can hold a value or nothing
        
        /**
         * @brief Get the best ask price (lowest sell price)
         * @return Best ask price, or std::nullopt if no asks exist
         */
        std::optional<Price> getBestAsk() const; //std::optional is a container that can hold a value or nothing
        
        /**
         * @brief Get the bid-ask spread
         * @return Spread (ask - bid), or std::nullopt if missing bid or ask
         */
        std::optional<Price> getSpread() const; //std::optional is a container that can hold a value or nothing
        
        /**
         * @brief Get total quantity at the best bid price
         * @return Quantity available at best bid, or 0 if no bids
         */
        Quantity getBestBidQuantity() const;
        
        /**
         * @brief Get total quantity at the best ask price
         * @return Quantity available at best ask, or 0 if no asks
         */
        Quantity getBestAskQuantity() const;
        
        /**
         * @brief Check if the order book is empty
         * @return true if no orders exist on either side
         */
        bool isOrderBookEmpty() const { return bids_.empty() && asks_.empty(); } //returns true if no orders exist on either side

        /**
         * @brief Get total number of orders in the book
         * @return Total order count across all price levels
         */
        size_t getOrderCount() const;
        

        /**
         * @brief Get number of price levels on bid side
         * @return Number of different bid prices
         */
        size_t getBidLevels() const { return bids_.size(); }
        
        /**
         * @brief Get number of price levels on ask side
         * @return Number of different ask prices
         */
        size_t getAskLevels() const { return asks_.size(); }
        
        /**
         * @brief Get order book state as string (for debugging)
         * @param max_levels Maximum number of price levels to show per side
         * @return Formatted string showing top of book
         */
        std::string toString(size_t max_levels = 5) const;
        
        /**
         * @brief Get all bid prices and quantities (for market data)
         * @param max_levels Maximum number of levels to return
         * @return Vector of (price, total_quantity) pairs, sorted by price descending
         */
        std::vector<std::pair<Price, Quantity>> getBidLevels(size_t max_levels = 10) const;
        
        /**
         * @brief Get all ask prices and quantities (for market data)
         * @param max_levels Maximum number of levels to return  
         * @return Vector of (price, total_quantity) pairs, sorted by price ascending
         */
        std::vector<std::pair<Price, Quantity>> getAskLevels(size_t max_levels = 10) const;
        
        /**
         * @brief Clear all orders from the book
         */
        void clear();
};

}