#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>
#include <queue>
#include <mutex>
#include "matching_engine/protocol.hpp"
#include "matching_engine/order.hpp"
#include "matching_engine/trade.hpp"
#include "matching_engine/matching_engine.hpp"

namespace matching_engine {

class Client {
public:
    using TradeCallback = std::function<void(const Trade&)>;
    using OrderCallback = std::function<void(const Order&)>;
    using ConnectionCallback = std::function<void(bool)>;

    Client(boost::asio::io_context& io_context);
    ~Client();

    // Connection management
    void connect(const std::string& host, unsigned short port);
    void disconnect();
    bool isConnected() const;

    // Order operations
    std::vector<Trade> submitOrder(const Order& order);
    bool cancelOrder(OrderId order_id, const std::string& symbol);
    bool modifyOrder(OrderId order_id, const std::string& symbol, Price new_price, Quantity new_quantity);

    // Market data queries
    std::optional<Price> getBestBid(const std::string& symbol);
    std::optional<Price> getBestAsk(const std::string& symbol);
    std::optional<Price> getSpread(const std::string& symbol);
    MarketDepth getMarketDepth(const std::string& symbol, size_t levels = 10);

    // Callbacks
    void setTradeCallback(TradeCallback callback);
    void setOrderCallback(OrderCallback callback);
    void setConnectionCallback(ConnectionCallback callback);

    // Utility
    std::string getConnectionStatus() const;

private:
    void doConnect(const std::string& host, unsigned short port);
    void doRead();
    void doWrite();
    void handleMessage(const Message& msg);
    void sendMessage(const Message& msg);
    void onConnect(boost::system::error_code ec);
    void onDisconnect();

    boost::asio::io_context& io_context_;
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
    
    // Message queue for sending
    std::queue<Message> send_queue_;
    std::mutex send_queue_mutex_;
    bool writing_ = false;

    // Callbacks
    TradeCallback trade_callback_;
    OrderCallback order_callback_;
    ConnectionCallback connection_callback_;

    // State
    bool connected_ = false;
    std::string host_;
    unsigned short port_;
};

} // namespace matching_engine 