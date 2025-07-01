#pragma once
#include <string>

namespace matching_engine {

enum class MessageType { // define a enum class to hold the message type
    ORDER,
    CANCEL,
    TRADE,
    UNKNOWN
};

struct Message { // define a struct to hold the message type and payload
    MessageType type;
    std::string payload;
};

// Convert MessageType to string
std::string messageTypeToString(MessageType type);

// Convert string to MessageType
MessageType stringToMessageType(const std::string& type_str);

// Serialize a message to a string
std::string serializeMessage(const Message& msg);

// Deserialize a string to a message
Message deserializeMessage(const std::string& data);

} // namespace matching_engine 