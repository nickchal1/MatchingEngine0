#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <unordered_map>
#include <vector>
#include "Order.h"

class OrderBook
{
private:
    std::map<Price, std::vector<OrderId>> m_bids;
    std::map<Price, std::vector<OrderId>> m_asks;
    std::unordered_map<OrderId, Order> m_idToOrder;
    OrderId m_nextId = 1;
public:
    bool cancelOrder(OrderId id);
    bool addOrder(Quantity quantity, Price price, Side side, OrderId& assignedId);
    void printOrderBook() const;
};

#endif