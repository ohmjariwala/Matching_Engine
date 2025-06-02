# Matching Engine

## Project Goal

This project aims to build a high-performance, real-time matching engine system implemented in C++ that facilitates efficient order matching for financial markets. The system will include both client and server components with networking capabilities, supporting market and limit orders with FIFO price-time priority matching algorithms.



**File: `include/matching_engine/types.hpp`**
- Defined type system with strong type aliases (`OrderId`, `Price`, `Quantity`, `TradeId`)
- Implemented scoped enumerations for type safety:
  - `OrderSide`: BUY/SELL with memory-efficient `uint8_t` backing
  - `OrderType`: MARKET/LIMIT orders
  - `OrderStatus`: Complete order lifecycle tracking (PENDING → ACTIVE → FILLED/CANCELLED)
  - `TradeSide`: Trade perspective tracking
- Added validation constants and utility functions:
  - Price and quantity validation ranges
  - String conversion utilities for debugging
  - Helper functions like `getOppositeSide()` for matching logic

**File: `include/matching_engine/order.hpp`**
- Started implementation of `Order` class with comprehensive functionality:
  - **Constructor Support**: Both limit orders (with price) and market orders (price = 0)
  - **State Management**: Tracks original quantity, remaining quantity, and fill status
  - **FIFO Support**: High-resolution timestamps for price-time priority
  - **Matching Logic**: `canMatchWith()` method with proper buy/sell price validation
  - **Priority Ordering**: `hasHigherPriorityThan()` implementing price-time priority
  - **Utility Methods**: Complete set of boolean checks (isMarketOrder, isBuyOrder, etc.)
  - **Partial Fills**: `fill()` method with validation and error handling


