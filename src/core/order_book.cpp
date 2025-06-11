#include "../../include/matching_engine/order_book.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace matching_engine {
    
// =============================================================================
// Public Methods
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
        oss << "  ASK " << std::fixed << std::setprecision(3) << price 
            << " [" << total_qty << " qty, " << queue.size() << " orders]" << std::endl;
        ask_count++;
    }
    
    // Display spread
    auto spread = getSpread();
    if (spread) {
        oss << "SPREAD: " << std::fixed << std::setprecision(3) << *spread << std::endl;
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
        oss << "  BID " << std::fixed << std::setprecision(3) << price 
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
    
    // Market buy orders match against asks (lowest prices first)
    // Market sell orders match against bids (highest prices first)
    if (market_order.isBuyOrder()) {
        // Match against asks - lowest prices first
        while (market_order.getRemainingQuantity() > 0 && !asks_.empty()) { //while the market order has remaining quantity and there are asks
            auto& best_price_level = asks_.begin()->second;
            if (best_price_level.empty()) { //if the best price level is empty
                asks_.erase(asks_.begin()); //remove the best price level
                continue;
            }
            
            Order& best_order = best_price_level.front();
            Price execution_price = determineExecutionPrice(market_order, best_order);
            
            // Calculate trade quantity (minimum of remaining quantities)
            Quantity trade_qty = std::min(market_order.getRemainingQuantity(),
                                        best_order.getRemainingQuantity()); //get the minimum of the remaining quantities
            
            // Create and store trade
            Trade trade = createTrade(market_order, best_order, execution_price, trade_qty);
            trades.push_back(trade); //add the trade to the trades vector
            
            // Fill both orders
            market_order.fill(trade_qty);
            best_order.fill(trade_qty);
            
            // Remove fully filled order from book
            if (best_order.isFullyFilled()) {
                order_locations_.erase(best_order.getId());
                best_price_level.pop();
                
                // Remove empty price level
                if (best_price_level.empty()) {
                    asks_.erase(asks_.begin());
                }
            }
        }
    } else {
        // Market sell order - match against bids (highest prices first)
        while (market_order.getRemainingQuantity() > 0 && !bids_.empty()) {
            auto& best_price_level = bids_.begin()->second;
            if (best_price_level.empty()) { //if the best price level is empty
                bids_.erase(bids_.begin()); //remove the best price level
                continue;
            }
            
            Order& best_order = best_price_level.front();
            Price execution_price = determineExecutionPrice(market_order, best_order);
            
            // Calculate trade quantity (minimum of remaining quantities)
            Quantity trade_qty = std::min(market_order.getRemainingQuantity(),
                                        best_order.getRemainingQuantity());
            
            // Create and store trade
            Trade trade = createTrade(best_order, market_order, execution_price, trade_qty);
            trades.push_back(trade); //add the trade to the trades vector
            
            // Fill both orders
            market_order.fill(trade_qty);
            best_order.fill(trade_qty);
            
            // Remove fully filled order from book
            if (best_order.isFullyFilled()) {
                order_locations_.erase(best_order.getId());
                best_price_level.pop();
                
                // Remove empty price level
                if (best_price_level.empty()) {
                    bids_.erase(bids_.begin());
                }
            }
        }
    }
    
    return trades;
}


std::vector<Trade> OrderBook::matchLimitOrder(Order& limit_order) {
    std::vector<Trade> trades;
    
    // Limit buy orders match against asks if the ask price <= limit price
    // Limit sell orders match against bids if the bid price >= limit price
    if (limit_order.isBuyOrder()) {
        // Match against asks - check if ask price <= limit price
        while (limit_order.getRemainingQuantity() > 0 && !asks_.empty()) { //while the limit order has remaining quantity and there are asks
            auto& best_price_level = asks_.begin()->second;
            if (best_price_level.empty()) { //if the best price level is empty
                asks_.erase(asks_.begin()); //remove the best price level
                continue;
            }
            
            Order& best_ask = best_price_level.front();
            
            // Check if limit order can match with this ask
            if (!limit_order.canMatchWith(best_ask)) {
                break; // No more matches possible at this price or better
            }
            
            Price execution_price = determineExecutionPrice(limit_order, best_ask);
            
            // Calculate trade quantity (minimum of remaining quantities)
            Quantity trade_qty = std::min(limit_order.getRemainingQuantity(), 
                                        best_ask.getRemainingQuantity());
            
            // Create and store trade
            Trade trade = createTrade(limit_order, best_ask, execution_price, trade_qty);
            trades.push_back(trade);
            
            // Fill both orders
            limit_order.fill(trade_qty);
            best_ask.fill(trade_qty);
            
            // Remove fully filled order from book
            if (best_ask.isFullyFilled()) {
                order_locations_.erase(best_ask.getId());
                best_price_level.pop();
                
                // Remove empty price level
                if (best_price_level.empty()) {
                    asks_.erase(asks_.begin());
                }
            }
        }
    } else {
        // Limit sell order - match against bids if bid price >= limit price
        while (limit_order.getRemainingQuantity() > 0 && !bids_.empty()) {
            auto& best_price_level = bids_.begin()->second;
            if (best_price_level.empty()) { //if the best price level is empty
                bids_.erase(bids_.begin()); //remove the best price level
                continue;
            }
            
            Order& best_bid = best_price_level.front();
            
            // Check if limit order can match with this bid
            if (!limit_order.canMatchWith(best_bid)) {
                break; // No more matches possible at this price or better
            }
            
            Price execution_price = determineExecutionPrice(limit_order, best_bid);
            
            // Calculate trade quantity (minimum of remaining quantities)
            Quantity trade_qty = std::min(limit_order.getRemainingQuantity(), 
                                        best_bid.getRemainingQuantity());
            
            // Create and store trade
            Trade trade = createTrade(best_bid, limit_order, execution_price, trade_qty);
            trades.push_back(trade);
            
            // Fill both orders
            limit_order.fill(trade_qty);
            best_bid.fill(trade_qty);
            
            // Remove fully filled order from book
            if (best_bid.isFullyFilled()) {
                order_locations_.erase(best_bid.getId());
                best_price_level.pop();
                
                // Remove empty price level
                if (best_price_level.empty()) {
                    bids_.erase(bids_.begin());
                }
            }
        }
    }
    
    return trades;
}

void OrderBook::addToBook(const Order& order) {
    // Add order to appropriate price level based on side
    if (order.isBuyOrder()) {
        bids_[order.getPrice()].push(order);
    } else {
        asks_[order.getPrice()].push(order);
    }
    
    // Add to order_locations_ for fast lookup during cancellation
    order_locations_[order.getId()] = {order.getPrice(), order.getSide()};
}

bool OrderBook::removeFromPriceLevel(Price price, OrderSide side, OrderId order_id) {
    std::queue<Order>* orders_queue_ptr = nullptr; //pointer to the orders queue
    
    // Get the appropriate queue based on order side
    if (side == OrderSide::BUY) {
        auto price_level_it = bids_.find(price); //find the price level iterator
        if (price_level_it == bids_.end()) {
            return false; // Price level not found
        }
        orders_queue_ptr = &(price_level_it->second);
    } else {
        auto price_level_it = asks_.find(price); //find the price level iterator
        if (price_level_it == asks_.end()) {
            return false; // Price level not found
        }
        orders_queue_ptr = &(price_level_it->second);
    }
    
    auto& orders_queue = *orders_queue_ptr; //get the orders queue
    
    // Since std::queue doesn't support removal from middle, we need to pop all orders, skip the target, and push the rest back

    
    std::queue<Order> temp_queue; //temporary queue
    bool found = false; //flag to check if the order is found
    
    while (!orders_queue.empty()) {
        Order current_order = orders_queue.front();
        orders_queue.pop();
        
        if (current_order.getId() == order_id) {
            found = true; // Skip this order (don't add to temp_queue)
        } else {
            temp_queue.push(current_order); // Keep this order
        }
    }
    
    orders_queue = std::move(temp_queue); //put the remaining orders back into the orders queue
    
    // If the price level is now empty, remove it from the map
    if (orders_queue.empty()) {
        if (side == OrderSide::BUY) {
            bids_.erase(price);
        } else {
            asks_.erase(price);
        }
    }
    
    return found;
}

// =============================================================================
// Helper Functions for Trade Creation
// =============================================================================

Trade OrderBook::createTrade(const Order& buy_order, const Order& sell_order, Price execution_price, Quantity quantity) {
    // Create a Trade object using generateTradeId(), and pass order IDs, price, quantity
    return Trade(generateTradeId(), buy_order.getId(), sell_order.getId(), 
                execution_price, quantity);
}

Price OrderBook::determineExecutionPrice(const Order& aggressive_order, const Order& passive_order) {
    // Passive order (already in book) gets price priority
    return passive_order.getPrice();
}

} // namespace matching_engine 