#include "matching_engine/server.hpp"
#include <iostream>

namespace matching_engine {

Server::Server(boost::asio::io_context& io_context, unsigned short port, MessageHandler handler) // constructor
    : io_context_(io_context),
      acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      message_handler_(std::move(handler)),
      running_(false) {}

void Server::start() { // start the server
    running_ = true;
    doAccept();
}

void Server::stop() { // stop the server
    running_ = false;
    boost::system::error_code ec;
    acceptor_.close(ec);
}

void Server::doAccept() { // accept a new connection
    if (!running_) return;
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_);
    acceptor_.async_accept(*socket, [this, socket](boost::system::error_code ec) {
        if (!ec) {
            doRead(socket);
        }
        doAccept();
    });
}

void Server::doRead(std::shared_ptr<boost::asio::ip::tcp::socket> socket) { // read a message from the socket
    auto buffer = std::make_shared<boost::asio::streambuf>();
    boost::asio::async_read_until(*socket, *buffer, '\n',
        [this, socket, buffer](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
            if (!ec) {
                std::istream is(buffer.get());
                std::string line;
                std::getline(is, line);
                if (!line.empty()) {
                    Message msg = deserializeMessage(line);
                    message_handler_(msg, socket);
                }
                doRead(socket); // Continue reading
            } else {
                socket->close();
            }
        });
}

} // namespace matching_engine 