#include "../../include/matching_engine/matching_engine.hpp"
#include "matching_engine/trade.hpp"
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace matching_engine {

MatchingEngine::MatchingEngine(const EngineConfig& config)
    : config_(config), total_orders_processed_(0), total_trades_executed_(0), is_running_(false) { //initialize the engine with the config
    start_time_ = std::chrono::high_resolution_clock::now();
}

MatchingEngine::~MatchingEngine() { //destructor
    stop();
}

void MatchingEngine::start() { //start the engine
    std::unique_lock lock(engine_mutex_);
    is_running_ = true;
    start_time_ = std::chrono::high_resolution_clock::now();
}

void MatchingEngine::stop() { //stop the engine
    std::unique_lock lock(engine_mutex_);
    is_running_ = false;
}

std::vector<Trade> MatchingEngine::submitOrder(Order order) {
    std::unique_lock lock(engine_mutex_); //lock the engine mutex- only one thread can access the engine at a time
    if (!is_running_) {
        throw std::runtime_error("Engine is not running"); 
    }
    if (!validateOrder(order)) {
        throw std::invalid_argument("Order validation failed");
    }
    auto* book = getOrderBook(order.getSymbol());
    if (!book) {
        throw std::runtime_error("Symbol not found: " + order.getSymbol());
    }
    auto trades = book->addOrder(order); //add the order to the order book
    total_orders_processed_++; //increment the total number of orders processed
    total_trades_executed_ += trades.size(); //increment the total number of trades executed
    for (const auto& trade : trades) { //broadcast the trades to all registered callbacks
        broadcastTrade(trade);
    }
    broadcastOrderUpdate(order); //broadcast the order to all registered callbacks
    return trades;
}

bool MatchingEngine::cancelOrder(OrderId order_id, const std::string& symbol) { 
    std::unique_lock lock(engine_mutex_); //lock the engine mutex- only one thread can access the engine at a time
    auto it = order_books_.find(symbol); //find the order book for the symbol
    if (it == order_books_.end()) {
        return false; // Symbol not found
    }
    bool result = it->second->cancelOrder(order_id); //cancel the order, second parameter is the order id
    if (result) {
        broadcastOrderUpdate(Order(order_id, symbol, OrderSide::BUY, OrderType::LIMIT, 0, 0)); // Dummy order for callback *****REVIEW*****
    }
    return result;
}

bool MatchingEngine::modifyOrder(OrderId order_id, const std::string& symbol, Price new_price, Quantity new_quantity) { 
    std::unique_lock lock(engine_mutex_); //lock the engine mutex- only one thread can access the engine at a time
    auto it = order_books_.find(symbol); //find the order book for the symbol
    if (it == order_books_.end()) {
        return false;
    }
    // Cancel and re-submit as new order (simple approach)
    bool cancelled = it->second->cancelOrder(order_id); //cancel the order, second parameter is the order id
    if (!cancelled) {
        return false;
    }
    //create a new order with the new price and quantity
    Order new_order(order_id, symbol, OrderSide::BUY, OrderType::LIMIT, new_price, new_quantity); // Assumes BUY, real impl should track side/type *****REVIEW*****
    auto trades = it->second->addOrder(new_order); //add the order to the order book
    //broadcast the trades to all registered callbacks
    for (const auto& trade : trades) { 
        broadcastTrade(trade);
    }
    broadcastOrderUpdate(new_order);
    return true;
}

std::optional<Price> MatchingEngine::getBestBid(const std::string& symbol) const { 
    std::shared_lock lock(engine_mutex_); //unique vs shared lock - unique lock is used to lock the engine mutex- only one thread can access the engine at a time, shared lock is used to lock the engine mutex- multiple threads can access the engine at a time
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) return std::nullopt; //if the order book is not found, return nullopt
    return it->second->getBestBid(); //return the best bid for the symbol
}

std::optional<Price> MatchingEngine::getBestAsk(const std::string& symbol) const {
    std::shared_lock lock(engine_mutex_); 
    auto it = order_books_.find(symbol); 
    if (it == order_books_.end()) return std::nullopt; //if the order book is not found, return nullopt
    return it->second->getBestAsk(); //return the best ask for the symbol
}

std::optional<Price> MatchingEngine::getSpread(const std::string& symbol) const {
    std::shared_lock lock(engine_mutex_);
    auto it = order_books_.find(symbol); 
    if (it == order_books_.end()) return std::nullopt; //if the order book is not found, return nullopt
    return it->second->getSpread(); //return the spread for the symbol
}

MarketDepth MatchingEngine::getMarketDepth(const std::string& symbol, size_t levels) const {
    std::shared_lock lock(engine_mutex_);
    MarketDepth depth; //create a new market depth object
    depth.symbol = symbol; //set the symbol for the market depth
    auto it = order_books_.find(symbol); //find the order book for the symbol
    if (it == order_books_.end()){
        return depth; //if the order book is not found, return the market depth
    } 
    depth.bids = it->second->getBidLevels(levels); //get the bid levels for the symbol
    depth.asks = it->second->getAskLevels(levels); //get the ask levels for the symbol
    depth.best_bid = it->second->getBestBid(); 
    depth.best_ask = it->second->getBestAsk(); 
    depth.spread = it->second->getSpread(); 
    depth.total_orders = it->second->getOrderCount(); //get the total number of orders for the symbol
    depth.timestamp = std::chrono::high_resolution_clock::now(); //get the timestamp for the market depth
    return depth; //return the market depth object
}

std::vector<std::string> MatchingEngine::getActiveSymbols() const {
    std::shared_lock lock(engine_mutex_);
    std::vector<std::string> symbols; //create a new vector of strings
    for (const auto& [symbol, _] : order_books_) { //iterate through the order books
        symbols.push_back(symbol); //add the symbol to the vector
    }
    return symbols; //return the vector of active symbols
}

void MatchingEngine::addSymbol(const std::string& symbol) {
    std::unique_lock lock(engine_mutex_); //lock the engine mutex- only one thread can access the engine at a time
    if (order_books_.count(symbol) == 0) { //if the symbol is not found, create a new order book for the symbol
        order_books_[symbol] = std::make_unique<OrderBook>();//creates order book object and wraps it in a unique pointer, which is a smart pointer that automatically manages the memory of the object
    }
}

bool MatchingEngine::removeSymbol(const std::string& symbol) {
    std::unique_lock lock(engine_mutex_);
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) return false;
    if (it->second->getOrderCount() > 0){
        return false; // Can't remove if orders exist
    } 
    order_books_.erase(it); //remove the order book for the symbol
    return true; //return true if the order book is removed
}

void MatchingEngine::registerTradeCallback(std::function<void(const Trade&)> callback) {
    std::unique_lock lock(engine_mutex_);
    trade_callbacks_.push_back(std::move(callback)); //add the callback to the vector of trade callbacks
}

void MatchingEngine::registerOrderCallback(std::function<void(const Order&)> callback) {
    std::unique_lock lock(engine_mutex_);
    order_callbacks_.push_back(std::move(callback)); //add the callback to the vector of order callbacks
}

void MatchingEngine::unregisterAllCallbacks() {
    std::unique_lock lock(engine_mutex_);
    trade_callbacks_.clear(); //clear the vector of trade callbacks
    order_callbacks_.clear(); //clear the vector of order callbacks
}

EngineStatistics MatchingEngine::getStatistics() const { 
    std::shared_lock lock(engine_mutex_);
    EngineStatistics stats; //create a new engine statistics object
    stats.total_orders_processed = total_orders_processed_; //set the total number of orders processed
    stats.total_trades_executed = total_trades_executed_; //set the total number of trades executed
    stats.total_symbols_active = order_books_.size(); //set the total number of symbols active
    stats.uptime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time_); //set the uptime
    // Latency, orders/sec, trades/sec can be calculated here if needed
    stats.average_latency_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time_).count(); // calculate the average latency in microseconds
    stats.orders_per_second = total_orders_processed_ / stats.uptime.count(); //set the orders per second
    stats.trades_per_second = total_trades_executed_ / stats.uptime.count(); //set the trades per second
    return stats; //return the engine statistics object
}

std::string MatchingEngine::getEngineStatus() const {
    auto stats = getStatistics(); //get the engine statistics
    std::ostringstream oss; //create a new string stream
    oss << "Engine running: " << (is_running_ ? "YES" : "NO") << "\n"; //set the engine running status
    oss << "Symbols: " << stats.total_symbols_active << ", Orders: " << stats.total_orders_processed << ", Trades: " << stats.total_trades_executed << "\n"; //set the total number of symbols, orders, and trades
    oss << "Uptime (ms): " << stats.uptime.count(); //set the uptime (how long the engine has been running)
    return oss.str();
}

void MatchingEngine::resetStatistics() {
    std::unique_lock lock(engine_mutex_);
    total_orders_processed_ = 0;
    total_trades_executed_ = 0;

    start_time_ = std::chrono::high_resolution_clock::now();
}

void MatchingEngine::updateConfig(const EngineConfig& config) {
    std::unique_lock lock(engine_mutex_);
    config_ = config; //update the config object
}

EngineConfig MatchingEngine::getConfig() const {
    std::shared_lock lock(engine_mutex_);
    return config_; //return the config object
}

std::string MatchingEngine::getOrderBookState(const std::string& symbol, size_t max_levels) const {
    std::shared_lock lock(engine_mutex_);
    auto it = order_books_.find(symbol); //find the order book for the symbol
    if (it == order_books_.end()) return "Symbol not found";
    return it->second->toString(max_levels); //return the order book state for the symbol
}

void MatchingEngine::clearAllOrderBooks() {
    std::unique_lock lock(engine_mutex_);
    order_books_.clear();
}

// --- Private helpers ---
bool MatchingEngine::validateOrder(const Order& order) const {
    if (!validateSymbol(order.getSymbol())){
        return false;
    } 
    if (order.getPrice() > config_.max_order_price){
        return false;
    } 
    if (order.getQuantity() > config_.max_order_quantity){
        return false;
    } 
    return true; //return true if the order is valid
}

bool MatchingEngine::validateSymbol(const std::string& symbol) const {
    if (symbol.empty() || symbol.size() > 8){
        return false;
    } 
    for (char c : symbol) {
        if (!std::isalnum(c)){
            return false;
        } 
    }
    return true; //return true if the symbol is valid
}

bool MatchingEngine::checkRiskLimits(const Order& order) const {
    // Check if order exceeds per-symbol limits
    if (order.getPrice() > config_.max_order_price){
        return false;
    } 
    if (order.getQuantity() > config_.max_order_quantity){
        return false;
    } 

    // Check if the number of orders for this symbol exceeds the limit
    auto it = order_books_.find(order.getSymbol());
    if (it != order_books_.end()) {
        if (it->second->getOrderCount() >= config_.max_orders_per_symbol){
            return false;
        }
    }

    // Check if the number of symbols exceeds the global limit
    if (order_books_.size() > config_.max_symbols){
        return false;
    }

    // Add more risk checks as needed (e.g., position limits, notional limits)
    return true; //return true if the order is valid
}

OrderBook* MatchingEngine::getOrderBook(const std::string& symbol) {
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) return nullptr;
    return it->second.get();
}

void MatchingEngine::cleanupEmptyOrderBooks() {
    for (auto it = order_books_.begin(); it != order_books_.end(); ) {
        if (it->second->getOrderCount() == 0) {
            it = order_books_.erase(it);
        } else {
            ++it;
        }
    }
}


void MatchingEngine::broadcastTrade(const Trade& trade) {
    for (const auto& cb : trade_callbacks_) {
        cb(trade);
    }
}

void MatchingEngine::broadcastOrderUpdate(const Order& order) {
    for (const auto& cb : order_callbacks_) {
        cb(order);
    }
}

} // namespace matching_engine 