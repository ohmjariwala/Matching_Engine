#pragma once

#include "types.hpp"
#include "order.hpp"
#include "order_book.hpp"
#include "matching_engine/trade.hpp"
#include <unordered_map> //for the order books
#include <memory> //for the order books
#include <vector> //for the trades
#include <functional> //for the callbacks
#include <atomic> //for the statistics
#include <chrono> //for stats
#include <shared_mutex> 
#include <optional> 
#include <string> 

namespace matching_engine {

// Forward declarations
struct EngineConfig; 
struct EngineStatistics;
struct MarketDepth;

/**
 * @brief Configuration parameters for the matching engine
 * This struct dictates the behavior and limits of the engine
 */
struct EngineConfig { //values are fixed for testing purposes

    // Risk limits
    Price max_order_price = 1000000.0; //fixed
    Quantity max_order_quantity = 1000000;
    size_t max_orders_per_symbol = 10000;
    
    // Performance settings
    bool enable_threading = true; //engine will use threads to process the orders
    size_t max_symbols = 1000;
    
    // Validation settings
    bool strict_validation = true; //engine will not accept orders that do not meet the risk limits
    bool enable_logging = true; //enable logging means that the engine will log the orders and trades to the console
    
    // Timeout settings
    std::chrono::milliseconds order_timeout = std::chrono::milliseconds(5000);
};

/**
 * @brief Engine performance statistics
 * This struct dictates the performance of the engine
 */
struct EngineStatistics { //values are fixed for testing purposes
    uint64_t total_orders_processed = 0; 
    uint64_t total_trades_executed = 0;
    uint64_t total_symbols_active = 0;
    double average_latency_microseconds = 0.0;
    double orders_per_second = 0.0;
    double trades_per_second = 0.0;
    std::chrono::milliseconds uptime = std::chrono::milliseconds(0);
    std::chrono::high_resolution_clock::time_point start_time;
};

/**
 * @brief Market depth information for a symbol
 * This struct is used to store the market depth for a symbol
 */
struct MarketDepth {
    std::string symbol;
    std::vector<std::pair<Price, Quantity>> bids;  // Sorted highest to lowest
    std::vector<std::pair<Price, Quantity>> asks;  // Sorted lowest to highest
    std::optional<Price> best_bid;
    std::optional<Price> best_ask;
    std::optional<Price> spread;
    size_t total_orders = 0;
    std::chrono::high_resolution_clock::time_point timestamp;
};



/**
 * @brief MatchingEngine: Main matching engine orchestrator managing multiple order books
 * 
 * The MatchingEngine class provides:
 * - Multi-symbol order book management (one OrderBook per symbol)
 * - Order routing and validation
 * - Trade execution and reporting
 * - Thread-safe concurrent access 
 * - Performance monitoring and statistics
 * 
 * Thread Safety:
 * - Uses reader-writer locks for concurrent access
 * - Read operations (market data) can run concurrently
 * - Write operations (order submission) are serialized per symbol
 */

class MatchingEngine {
private:
    // =============================================================================
    // Core Data Members
    // =============================================================================
    
    // Multi-symbol order book management
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> order_books_; //hashmap with symbol as key and order book as value (through unique pointer)
    
    // Event callbacks
    std::vector<std::function<void(const Trade&)>> trade_callbacks_; //vector of functions that take a const Trade& as an argument
    std::vector<std::function<void(const Order&)>> order_callbacks_; //vector of functions that take a const Order& as an argument
    
    // Engine statistics and monitoring
    //atomic integer - thread safe integer
    std::atomic<uint64_t> total_orders_processed_; 
    std::atomic<uint64_t> total_trades_executed_;
    std::chrono::high_resolution_clock::time_point start_time_;
    
    // Configuration and thread safety
    EngineConfig config_; //config for the engine - stores all settings and limits
    mutable std::shared_mutex engine_mutex_;  // Reader-writer lock, meaning that multiple threads can read the data but only one thread can write to the data
    
    // Engine state
    //atomic boolean - thread safe boolean
    std::atomic<bool> is_running_; //atomic boolean to store the state of the engine
    
    // =============================================================================
    // Private Helper Methods
    // =============================================================================
    
    /**
     * @brief Validate order before processing
     * @param order The order to validate
     * @return true if order is valid
     */
    bool validateOrder(const Order& order) const; //validate the order before processing
    
    /**
     * @brief Validate symbol format and constraints
     * @param symbol The symbol to validate
     * @return true if symbol is valid
     */
    bool validateSymbol(const std::string& symbol) const; //validate the symbol before processing
    
    /**
     * @brief Check risk limits for order
     * @param order The order to check
     * @return true if within risk limits
     */
    bool checkRiskLimits(const Order& order) const; //check the risk limits for the order
    
    /**
     * @brief Get or create order book for symbol
     * @param symbol The symbol to get order book for
     * @return Pointer to order book (never null)
     */
    OrderBook* getOrderBook(const std::string& symbol); //get the order book for the symbol
    
    /**
     * @brief Remove empty order books to free memory
     */
    void cleanupEmptyOrderBooks(); //remove empty order books to free memory
    
    /**
     * @brief Broadcast trade to all registered callbacks
     * @param trade The trade to broadcast
     */
    void broadcastTrade(const Trade& trade); //broadcast the trade to all registered callbacks
    
    /**
     * @brief Broadcast order update to all registered callbacks
     * @param order The order to broadcast
     */
    void broadcastOrderUpdate(const Order& order); //broadcast the order update to all registered callbacks



public:
    // =============================================================================
    // Constructor & Destructor
    // =============================================================================
    
    /**
     * @brief Construct a new Matching Engine
     * @param config Engine configuration parameters
     * the explicit keyword is used to prevent accidental implicit conversions, meaning that the constructor will only be called with an EngineConfig object and not with a temporary object or a different type
     */
    explicit MatchingEngine(const EngineConfig& config = EngineConfig{}); //constructor
    
    /**
     * @brief Destroy the Matching Engine
     * the destructor is called when the engine is destroyed
     */
    ~MatchingEngine(); //destructor
    
    // =============================================================================
    // Engine Management
    // =============================================================================
    
    /**
     * @brief Starts machine
     */
    void start();
    
    /**
     * @brief Stop machine 
     */
    void stop();
    
    /**
     * @brief Check if engine is running
     * @return true if engine is active
     */
    bool isRunning() const { return is_running_.load(); }
    
    // =============================================================================
    // Order Management
    // =============================================================================
    
    /**
     * @brief Submit an order to the matching engine
     * @param order The order to submit
     * @return Vector of trades executed (empty if no matches)
     */

    std::vector<Trade> submitOrder(Order order);
    
    /**
     * @brief Cancel an existing order
     * @param order_id The ID of the order to cancel
     * @param symbol The symbol of the order
     * @return true if order was found and cancelled
     */

    bool cancelOrder(OrderId order_id, const std::string& symbol);
    
    /**
     * @brief Modify an existing order (cancel and replace)
     * @param order_id The ID of the order to modify
     * @param symbol The symbol of the order
     * @param new_price New price for the order
     * @param new_quantity New quantity for the order
     * @return true if order was found and modified
     */

    bool modifyOrder(OrderId order_id, const std::string& symbol, Price new_price, Quantity new_quantity); //modify an existing order
    
    // =============================================================================
    // Getting Market Data
    // =============================================================================
    
    /**
     * @brief Get best bid price for a symbol
     * @param symbol The symbol to query
     * @return Best bid price or nullopt if no bids
     */
    std::optional<Price> getBestBid(const std::string& symbol) const;
    
    /**
     * @brief Get best ask price for a symbol
     * @param symbol The symbol to query
     * @return Best ask price or nullopt if no asks
     */
    std::optional<Price> getBestAsk(const std::string& symbol) const;
    
    /**
     * @brief Get bid-ask spread for a symbol
     * @param symbol The symbol to query
     * @return Spread or nullopt if no bid/ask
     */
    std::optional<Price> getSpread(const std::string& symbol) const;
    
    /**
     * @brief Get market depth for a symbol
     * @param symbol The symbol to query
     * @param levels Number of price levels to include
     * @return Market depth information
     */
    MarketDepth getMarketDepth(const std::string& symbol, size_t levels = 5) const;
    
    /**
     * @brief Get list of all active symbols
     * @return Vector of symbol names
     */
    std::vector<std::string> getActiveSymbols() const;
    
    // =============================================================================
    // Adding and Removing Symbols
    // =============================================================================
    
    /**
     * @brief Add a new trading symbol
     * @param symbol The symbol to add
     */
    void addSymbol(const std::string& symbol);
    
    /**
     * @brief Remove a trading symbol (only if no active orders)
     * @param symbol The symbol to remove
     * @return true if symbol was removed successfully
     */
    bool removeSymbol(const std::string& symbol);
    
    // =============================================================================
    // Event Handling & Callbacks
    // =============================================================================
    
    /**
     * @brief Register a callback for trade events
     * @param callback Function to call when trades execute
     */
    void registerTradeCallback(std::function<void(const Trade&)> callback);
    
    /**
     * @brief Register a callback for order events
     * @param callback Function to call when orders are updated
     */
    void registerOrderCallback(std::function<void(const Order&)> callback);
    
    /**
     * @brief Remove all registered callbacks
     */
    void unregisterAllCallbacks();
    
    // =============================================================================
    // Statistics & Monitoring
    // =============================================================================
    
    /**
     * @brief Get engine performance statistics
     * @return Current statistics snapshot
     */
    EngineStatistics getStatistics() const;
    
    /**
     * @brief Get engine status as formatted string
     * @return Status report for monitoring
     */
    std::string getEngineStatus() const;
    
    /**
     * @brief Reset all statistics counters
     */
    void resetStatistics();
    
    // =============================================================================
    // Configuration Management
    // =============================================================================
    
    /**
     * @brief Update engine configuration
     * @param config New configuration parameters
     */
    void updateConfig(const EngineConfig& config);
    
    /**
     * @brief Get current engine configuration
     * @return Current configuration
     */
    EngineConfig getConfig() const;
    
    // =============================================================================
    // Debugging & Utilities
    // =============================================================================
    
    /**
     * @brief Get order book state for a symbol
     * @param symbol The symbol to query
     * @param max_levels Maximum price levels to show
     * @return Formatted order book string
     */
    std::string getOrderBookState(const std::string& symbol, size_t max_levels = 5) const;
    
    /**
     * @brief Clear all order books to reset engine (use in case of issues and testing only, not in production)
     */
    void clearAllOrderBooks();
};

} // namespace matching_engine 