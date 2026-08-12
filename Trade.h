#ifndef TRADE_H
#define TRADE_H

#include "Order.h"

struct Trade
{
    OrderId makerId;
    OrderId takerId;
    Quantity executionQuantity;
    Price executionPrice;
    Side takerSide;
};

#endif
