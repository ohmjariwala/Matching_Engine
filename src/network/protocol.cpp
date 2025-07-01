#include "matching_engine/protocol.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <string>

namespace matching_engine {

MessageType stringToMessageType(const std::string& type_str) { // convert string to MessageType
    if (type_str == "ORDER") return MessageType::ORDER;
    if (type_str == "CANCEL") return MessageType::CANCEL;
    if (type_str == "TRADE") return MessageType::TRADE;
    return MessageType::UNKNOWN;
}

std::string messageTypeToString(MessageType type) { // convert MessageType to string
    switch (type) {
        case MessageType::ORDER: return "ORDER";
        case MessageType::CANCEL: return "CANCEL";
        case MessageType::TRADE: return "TRADE";
        default: return "UNKNOWN";
    }
}

std::string serializeMessage(const Message& msg) { // serialize a message to a string
    // Simple format: <type>|<payload>
    return messageTypeToString(msg.type) + "|" + msg.payload;
}

Message deserializeMessage(const std::string& data) { // deserialize a string to a message
    auto sep = data.find('|');
    if (sep == std::string::npos) return {MessageType::UNKNOWN, data};
    std::string type_str = data.substr(0, sep);
    std::string payload = data.substr(sep + 1);
    return {stringToMessageType(type_str), payload};
}

} // namespace matching_engine 