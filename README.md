# Matching Engine

A high-performance, real-time matching engine implemented in C++ supporting market and limit orders with FIFO price-time priority matching algorithms.


### **Completed Components**

**Core Engine**
- **`types.hpp`** - Complete type system with strong aliases, enums, validation constants
- **`order.hpp/cpp`** - Full Order class with matching logic, FIFO priority, partial fills
- **`order_book.hpp/cpp`** - Complete OrderBook with all matching algorithms implemented
- **`matching_engine.hpp/cpp`** - Complete MatchingEngine coordinating multiple order books

**Networking Layer**
- **`protocol.hpp/cpp`** - Message serialization/deserialization protocol
- **`server.hpp/cpp`** - Boost.asio TCP server for handling client connections
- **`client.hpp/cpp`** - Boost.asio TCP client for connecting to the engine

**Key Features Working**
-  **FIFO Price-Time Priority**: Proper order queue management within price levels
-  **Market & Limit Orders**: Full support for both order types with immediate execution
-  **Real-Time Matching**: `executeMarketOrder()` and `matchLimitOrder()` algorithms
-  **Order Management**: Add, cancel, and modify orders with O(1) lookup
-  **Trade Generation**: Complete trade execution with proper price priority rules
-  **Market Data**: Best bid/ask, spread calculation, order book depth
-  **Multi-Symbol Support**: Independent order books for different trading symbols
-  **Network Communication**: Client-server architecture with async I/O
-  **Callbacks & Events**: Real-time trade and order update notifications

###  **System Architecture**
```
include/matching_engine/
├── types.hpp           # Type system and constants  
├── order.hpp           # Order class declaration
├── order_book.hpp      # OrderBook class declaration
├── matching_engine.hpp # MatchingEngine class declaration
├── protocol.hpp        # Network protocol definitions
├── server.hpp          # TCP server class
└── client.hpp          # TCP client class

src/core/
├── order.cpp           # Order implementation
├── order_book.cpp      # OrderBook matching algorithms
└── matching_engine.cpp # Engine coordination logic

src/network/
├── protocol.cpp        # Message serialization
├── server.cpp          # TCP server implementation
└── client.cpp          # TCP client implementation
```

###  **Usage Example**
```cpp
// Start the server
boost::asio::io_context io_context;
MatchingEngine engine(config);
Server server(io_context, 8080, messageHandler);
server.start();

// Connect client
Client client(io_context);
client.connect("localhost", 8080);

// Submit orders
Order order(1, "AAPL", OrderSide::BUY, OrderType::LIMIT, 15000, 100);
auto trades = client.submitOrder(order);
```

**Data Structures**: Maps with custom comparators for price-time priority, hash maps for O(1) order lookup, FIFO queues within price levels, async network I/O with Boost.asio.


