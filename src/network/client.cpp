#include "matching_engine/client.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

namespace matching_engine {

Client::Client(boost::asio::io_context& io_context)
    : io_context_(io_context), socket_(std::make_unique<boost::asio::ip::tcp::socket>(io_context)) {
}

Client::~Client() {
    disconnect();
}

void Client::connect(const std::string& host, unsigned short port) {
    host_ = host;
    port_ = port;
    doConnect(host, port);
}

void Client::disconnect() {
    if (connected_ && socket_->is_open()) {
        boost::system::error_code ec;
        socket_->close(ec);
        onDisconnect();
    }
}

bool Client::isConnected() const {
    return connected_ && socket_ && socket_->is_open();
}

std::vector<Trade> Client::submitOrder(const Order& order) {
    if (!isConnected()) {
        throw std::runtime_error("Not connected to server");
    }

    // Serialize order to JSON-like format
    std::ostringstream oss;
    oss << "SUBMIT_ORDER|" 
        << order.getId() << ","
        << order.getSymbol() << ","
        << static_cast<int>(order.getSide()) << ","
        << static_cast<int>(order.getType()) << ","
        << order.getPrice() << ","
        << order.getQuantity();
    
    Message msg{MessageType::ORDER, oss.str()};
    sendMessage(msg);
    
    // For now, return empty trades - in a real implementation,
    // you'd wait for the server response
    return {};
}

bool Client::cancelOrder(OrderId order_id, const std::string& symbol) {
    if (!isConnected()) {
        return false;
    }

    std::ostringstream oss;
    oss << "CANCEL_ORDER|" << order_id << "," << symbol;
    
    Message msg{MessageType::CANCEL, oss.str()};
    sendMessage(msg);
    
    return true; // Assume success for now
}

bool Client::modifyOrder(OrderId order_id, const std::string& symbol, Price new_price, Quantity new_quantity) {
    if (!isConnected()) {
        return false;
    }

    std::ostringstream oss;
    oss << "MODIFY_ORDER|" << order_id << "," << symbol << "," << new_price << "," << new_quantity;
    
    Message msg{MessageType::ORDER, oss.str()};
    sendMessage(msg);
    
    return true; // Assume success for now
}

std::optional<Price> Client::getBestBid(const std::string& symbol) {
    if (!isConnected()) {
        return std::nullopt;
    }

    std::ostringstream oss;
    oss << "GET_BEST_BID|" << symbol;
    
    Message msg{MessageType::ORDER, oss.str()};
    sendMessage(msg);
    
    return std::nullopt; // Would need to implement response handling
}

std::optional<Price> Client::getBestAsk(const std::string& symbol) {
    if (!isConnected()) {
        return std::nullopt;
    }

    std::ostringstream oss;
    oss << "GET_BEST_ASK|" << symbol;
    
    Message msg{MessageType::ORDER, oss.str()};
    sendMessage(msg);
    
    return std::nullopt; // Would need to implement response handling
}

std::optional<Price> Client::getSpread(const std::string& symbol) {
    if (!isConnected()) {
        return std::nullopt;
    }

    std::ostringstream oss;
    oss << "GET_SPREAD|" << symbol;
    
    Message msg{MessageType::ORDER, oss.str()};
    sendMessage(msg);
    
    return std::nullopt; // Would need to implement response handling
}

MarketDepth Client::getMarketDepth(const std::string& symbol, size_t levels) {
    MarketDepth depth;
    depth.symbol = symbol;
    
    if (!isConnected()) {
        return depth;
    }

    std::ostringstream oss;
    oss << "GET_MARKET_DEPTH|" << symbol << "," << levels;
    
    Message msg{MessageType::ORDER, oss.str()};
    sendMessage(msg);
    
    return depth; // Would need to implement response handling
}

void Client::setTradeCallback(TradeCallback callback) {
    trade_callback_ = std::move(callback);
}

void Client::setOrderCallback(OrderCallback callback) {
    order_callback_ = std::move(callback);
}

void Client::setConnectionCallback(ConnectionCallback callback) {
    connection_callback_ = std::move(callback);
}

std::string Client::getConnectionStatus() const {
    std::ostringstream oss;
    oss << "Connected: " << (connected_ ? "YES" : "NO");
    if (connected_) {
        oss << " to " << host_ << ":" << port_;
    }
    return oss.str();
}

// Private methods

void Client::doConnect(const std::string& host, unsigned short port) {
    boost::asio::ip::tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    
    boost::asio::async_connect(*socket_, endpoints,
        [this](boost::system::error_code ec, const boost::asio::ip::tcp::endpoint&) {
            onConnect(ec);
        });
}

void Client::doRead() {
    auto buffer = std::make_shared<boost::asio::streambuf>();
    boost::asio::async_read_until(*socket_, *buffer, '\n',
        [this, buffer](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
            if (!ec) {
                std::istream is(buffer.get());
                std::string line;
                std::getline(is, line);
                if (!line.empty()) {
                    Message msg = deserializeMessage(line);
                    handleMessage(msg);
                }
                doRead(); // Continue reading
            } else {
                onDisconnect();
            }
        });
}

void Client::doWrite() {
    if (send_queue_.empty()) {
        writing_ = false;
        return;
    }

    Message msg = send_queue_.front();
    send_queue_.pop();
    
    std::string data = serializeMessage(msg) + "\n";
    auto buffer = std::make_shared<std::string>(std::move(data));
    
    boost::asio::async_write(*socket_, boost::asio::buffer(*buffer),
        [this, buffer](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
            if (!ec) {
                doWrite(); // Continue writing
            } else {
                onDisconnect();
            }
        });
}

void Client::handleMessage(const Message& msg) {
    switch (msg.type) {
        case MessageType::TRADE:
            if (trade_callback_) {
                // Parse trade from payload and call callback
                // This is a simplified implementation
                Trade trade{0, 0, 0, 0, 0, 0}; // Dummy trade
                trade_callback_(trade);
            }
            break;
            
        case MessageType::ORDER:
            if (order_callback_) {
                // Parse order from payload and call callback
                // This is a simplified implementation
                Order order{0, "", OrderSide::BUY, OrderType::LIMIT, 0, 0}; // Dummy order
                order_callback_(order);
            }
            break;
            
        default:
            std::cout << "Received unknown message: " << msg.payload << std::endl;
            break;
    }
}

void Client::sendMessage(const Message& msg) {
    {
        std::lock_guard<std::mutex> lock(send_queue_mutex_);
        send_queue_.push(msg);
    }
    
    if (!writing_) {
        writing_ = true;
        doWrite();
    }
}

void Client::onConnect(boost::system::error_code ec) {
    if (!ec) {
        connected_ = true;
        std::cout << "Connected to server at " << host_ << ":" << port_ << std::endl;
        doRead(); // Start reading messages
        if (connection_callback_) {
            connection_callback_(true);
        }
    } else {
        std::cerr << "Failed to connect: " << ec.message() << std::endl;
        if (connection_callback_) {
            connection_callback_(false);
        }
    }
}

void Client::onDisconnect() {
    connected_ = false;
    std::cout << "Disconnected from server" << std::endl;
    if (connection_callback_) {
        connection_callback_(false);
    }
}

} // namespace matching_engine 