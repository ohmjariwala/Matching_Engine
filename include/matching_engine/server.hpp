#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>
#include "matching_engine/protocol.hpp"

namespace matching_engine {

class Server { // define a class to hold the server
public:
    using MessageHandler = std::function<void(const Message&, std::shared_ptr<boost::asio::ip::tcp::socket>)>; // define a function to handle the message

    Server(boost::asio::io_context& io_context, unsigned short port, MessageHandler handler); // constructor
    void start(); // start the server
    void stop(); // stop the server

private:
    void doAccept(); // accept a new connection
    void doRead(std::shared_ptr<boost::asio::ip::tcp::socket> socket); // read a message from the socket

    boost::asio::io_context& io_context_; // io context -> this does the work of accepting new connections and reading data from them
    boost::asio::ip::tcp::acceptor acceptor_; // acceptor -> this is the object that accepts new connections
    MessageHandler message_handler_; // message handler -> this is the function that handles the message
    bool running_; // running -> this is the flag that indicates if the server is running
};

} // namespace matching_engine 