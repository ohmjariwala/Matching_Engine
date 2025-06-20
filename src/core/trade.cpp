#include "matching_engine/trade.hpp"
#include <sstream>

namespace matching_engine {

std::string Trade::toString() const {
    std::ostringstream oss;
    oss << "Trade " << trade_id << ": " << symbol << " " << quantity << " @ " << price;
    return oss.str();
}

} // namespace matching_engine 