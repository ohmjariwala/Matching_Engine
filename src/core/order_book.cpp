#include "../../include/matching_engine/order_book.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace matching_engine {

// =============================================================================
// Public Interface Methods
// =============================================================================

std::vector<Trade> OrderBook::addOrder(Order order) {
    std::vector<Trade> trades;
    
    if (order.isMarketOrder()) { //if the order is a market order
        trades = executeMarketOrder(order);
        // Market orders are never added to book - they execute immediately
    } else if (order.isLimitOrder()) { //if the order is a limit order
        trades = matchLimitOrder(order);
        
        // If the limit order is not fully filled, add remaining to the book
        if (!order.isFullyFilled()) {
            addToBook(order);
        }
    }

    // If trades were executed, return the trades
    return trades;
}

bool OrderBook::cancelOrder(OrderId order_id) {
    // Look up order in order_locations_
    auto it = order_locations_.find(order_id); //iterator to the order
    if (it == order_locations_.end()) { //if the order is not found
        return false; // Order not found
    }
    
    // Get price and side
    auto [price, side] = it->second; //get the price and side from the order
    
    // Remove from price level
    removeFromPriceLevel(price, side, order_id); //remove the order from the price level
    
    // Remove from order_locations_
    order_locations_.erase(it); //remove the order from the order_locations_
    
    return true; //return true if the order is cancelled
}

std::optional<Price> OrderBook::getBestBid() const {
    // bids_ uses std::greater<Price>, so .begin() gives highest price
    if (bids_.empty()) {
        return std::nullopt; //if there are no bids, return nullopt
    }
    return bids_.begin()->first; //return the highest bid price
}

std::optional<Price> OrderBook::getBestAsk() const {
    // asks_ map is sorted by price, use .begin() or .rbegin() depending on comparator
    if (asks_.empty()) {
        return std::nullopt; //if there are no asks, return nullopt
    }
    return asks_.begin()->first; //return the lowest ask price
}

std::optional<Price> OrderBook::getSpread() const {
    auto best_bid = getBestBid();
    auto best_ask = getBestAsk();
    
    if (best_bid && best_ask) {
        return *best_ask - *best_bid; //return the spread
    }
    return std::nullopt; //if there is no spread, return nullopt
}

Quantity OrderBook::getBestBidQuantity() const {
    auto best_bid = getBestBid();
    if (!best_bid) {
        return 0; //if there are no bids, return 0
    }
    
    // Find the price level and sum all quantities in the queue
    auto price_level_it = bids_.find(*best_bid);
    if (price_level_it == bids_.end()) {
        return 0; //if the price level is not found, return 0
    }
    
    const auto& orders_queue = price_level_it->second;
    Quantity total_quantity = 0;
    
    // Create a copy of the queue to iterate through it (since std::queue doesn't have iterators)
    auto temp_queue = orders_queue;
    while (!temp_queue.empty()) {
        total_quantity += temp_queue.front().getRemainingQuantity();
        temp_queue.pop();
    }
    
    return total_quantity;
}


Quantity OrderBook::getBestAskQuantity() const {        
    auto best_ask = getBestAsk();
    if (!best_ask) {
        return 0; //if there are no asks, return 0
    }
    
    auto price_level_it = asks_.find(*best_ask); //price level iterator: find the price level
    if (price_level_it == asks_.end()) {
        return 0; //if the price level is not found, return 0
    }
    
    const auto& orders_queue = price_level_it->second; //get the queue at the price level
    Quantity total_quantity = 0;

    auto temp_queue = orders_queue;
    while (!temp_queue.empty()) {
        total_quantity += temp_queue.front().getRemainingQuantity();
        temp_queue.pop();
    }
    
    return total_quantity;
}

size_t OrderBook::getOrderCount() const {
    size_t count = 0;
    
    // Iterate through both bids_ and asks_ maps
    for (const auto& [price, queue] : bids_) { //for each price level in bids_
        count += queue.size(); //add the number of orders in the queue to the count
    }
    for (const auto& [price, queue] : asks_) { //for each price level in asks_
        count += queue.size(); //add the number of orders in the queue to the count
    }
    
    return count; //return the total number of orders
}

std::string OrderBook::toString(size_t max_levels) const {
    std::ostringstream oss;

    // Display asks (lowest prices first, limited by max_levels)
    oss << "=== ORDER BOOK ===" << std::endl;
    oss << "ASKS (lowest first):" << std::endl;
    
    size_t ask_count = 0;
    for (const auto& [price, queue] : asks_) {
        if (ask_count >= max_levels) break;
        Quantity total_qty = 0;
        auto temp_queue = queue;
        while (!temp_queue.empty()) {
            total_qty += temp_queue.front().getRemainingQuantity();
            temp_queue.pop();
        }
        oss << "  ASK " << std::fixed << std::setprecision(2) << price 
            << " [" << total_qty << " qty, " << queue.size() << " orders]" << std::endl;
        ask_count++;
    }
    
    // Display spread
    auto spread = getSpread();
    if (spread) {
        oss << "SPREAD: " << std::fixed << std::setprecision(2) << *spread << std::endl;
    } else {
        oss << "SPREAD: N/A" << std::endl;
    }
    
    // Display bids (highest prices first, limited by max_levels)
    oss << "BIDS (highest first):" << std::endl;
    size_t bid_count = 0;
    for (const auto& [price, queue] : bids_) {
        if (bid_count >= max_levels) break;
        Quantity total_qty = 0;
        auto temp_queue = queue;
        while (!temp_queue.empty()) {
            total_qty += temp_queue.front().getRemainingQuantity();
            temp_queue.pop();
        }
        oss << "  BID " << std::fixed << std::setprecision(2) << price 
            << " [" << total_qty << " qty, " << queue.size() << " orders]" << std::endl;
        bid_count++;
    }
    
    oss << "=================" << std::endl;
    oss << "Total Orders: " << getOrderCount() << std::endl;
    
    return oss.str();
}

std::vector<std::pair<Price, Quantity>> OrderBook::getBidLevels(size_t max_levels) const {
    std::vector<std::pair<Price, Quantity>> levels;

    // Iterate through bids_ map, sum quantities at each price level
    for (const auto& [price, queue] : bids_) { //for each price level in bids_
        Quantity total_qty = 0;
        auto temp_queue = queue;
        while (!temp_queue.empty()) {
            total_qty += temp_queue.front().getRemainingQuantity();
            temp_queue.pop();
        }
        levels.push_back({price, total_qty}); //add the price and total quantity to the levels vector
        if (levels.size() >= max_levels) break; //if the number of levels is greater than or equal to max_levels, break
    }
    
    // Return the levels vector
    return levels;
}

std::vector<std::pair<Price, Quantity>> OrderBook::getAskLevels(size_t max_levels) const {
    std::vector<std::pair<Price, Quantity>> levels;
    // Iterate through asks_ map, sum quantities at each price level
    for (const auto& [price, queue] : asks_) { //for each price level in asks_
        Quantity total_qty = 0;
        auto temp_queue = queue;
        while (!temp_queue.empty()) {
            total_qty += temp_queue.front().getRemainingQuantity();
            temp_queue.pop();
        }
        levels.push_back({price, total_qty}); //add the price and total quantity to the levels vector
        if (levels.size() >= max_levels) break; //if the number of levels is greater than or equal to max_levels, break
    }

    // Return the levels vector
    return levels;
}

void OrderBook::clear() {
    // Clear bids_
    bids_.clear();
    asks_.clear();
    order_locations_.clear();
    next_trade_id_ = 0;
}

// =============================================================================
// Private Helper Methods
// =============================================================================

std::vector<Trade> OrderBook::executeMarketOrder(Order& market_order) {
    std::vector<Trade> trades;
    
    // TODO: Execute market order against best available prices
    // 1. If buy market order -> match against asks (lowest prices first)
    // 2. If sell market order -> match against bids (highest prices first)
    // 3. Keep matching until order is filled or no more liquidity
    
    // ALGORITHM HINT:
    // while (market_order.getRemainingQuantity() > 0 && !opposite_side_empty) {
    //     auto& best_price_level = get_best_opposite_side_queue();
    //     auto& best_order = best_price_level.front();
    //     
    //     // Calculate trade quantity (min of remaining quantities)
    //     // Create trade at best_order's price
    //     // Fill both orders
    //     // Remove fully filled orders from queue
    //     // If queue becomes empty, remove price level
    // }
    
    return trades;
}

std::vector<Trade> OrderBook::matchLimitOrder(Order& limit_order) {
    std::vector<Trade> trades;
    
    // TODO: Try to match limit order against existing orders
    // 1. Check if limit order can match with best price on opposite side
    // 2. If yes, execute trades (similar to market order logic)
    // 3. Stop when no more matches possible or order fully filled
    
    // ALGORITHM HINT:
    // while (limit_order.getRemainingQuantity() > 0) {
    //     auto best_opposite = get_best_opposite_price();
    //     if (!best_opposite || !limit_order.canMatchWith(best_opposite_order)) {
    //         break; // No more matches possible
    //     }
    //     
    //     // Execute trade (similar to market order logic)
    // }
    
    return trades;
}

void OrderBook::addToBook(const Order& order) {
    // TODO: Add order to appropriate price level
    // 1. Determine if it's a bid or ask
    // 2. Add to appropriate map (bids_ or asks_)
    // 3. Add to order_locations_ for fast lookup
    
    // HINT: 
    // if (order.isBuyOrder()) {
    //     bids_[order.getPrice()].push(order);
    // } else {
    //     asks_[order.getPrice()].push(order);
    // }
    // order_locations_[order.getId()] = {order.getPrice(), order.getSide()};
}

bool OrderBook::removeFromPriceLevel(Price price, OrderSide side, OrderId order_id) {
    // TODO: Remove specific order from a price level
    // 1. Get the appropriate map (bids_ or asks_)
    // 2. Find the price level
    // 3. Search through queue to find order with matching ID
    // 4. Remove the order (tricky with std::queue!)
    
    // CHALLENGE: std::queue doesn't support removal from middle
    // HINT: You might need to:
    // - Pop all orders into a temporary container
    // - Skip the target order
    // - Push remaining orders back
    // - Or consider using std::deque instead of std::queue
    
    return false; // Placeholder
}

// =============================================================================
// Helper Functions for Trade Creation
// =============================================================================

Trade OrderBook::createTrade(const Order& buy_order, const Order& sell_order, Price execution_price, Quantity quantity) {
    // TODO: Create a Trade object
    // HINT: Use generateTradeId(), and pass order IDs, price, quantity
    
    return Trade(generateTradeId(), buy_order.getId(), sell_order.getId(), 
                execution_price, quantity);
}

Price OrderBook::determineExecutionPrice(const Order& aggressive_order, const Order& passive_order) {
    // TODO: Determine execution price
    // RULE: Passive order (already in book) gets price priority
    // HINT: Return passive_order.getPrice()
    
    return passive_order.getPrice();
}

} // namespace matching_engine 