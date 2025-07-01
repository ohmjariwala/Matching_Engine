cmake_minimum_required(VERSION 3.10)
project(matching_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(matching_engine
    src/core/order.cpp
    src/core/matching_engine.cpp
    src/core/order_book.cpp
    src/core/trade.cpp
    src/network/protocol.cpp
    src/network/server.cpp
    src/network/client.cpp
)

target_include_directories(matching_engine PUBLIC include) 