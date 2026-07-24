#ifndef ORDER_H
#define ORDER_H

#include <cstdint>

enum class Side 
{
    Buy,
    Sell,
};

using OrderId = std::uint64_t;
using Price = std::int32_t;
using Quantity = std::int64_t;

struct Order
{
    OrderId id;
    Quantity quantity;
    Price price;
    Side side;
};

#endif