#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <unordered_map>
#include <vector>
#include <optional>
#include "SubmissionResult.h"
#include "Order.h"
#include "Trade.h"

class OrderBook
{
private:
    std::map<Price, std::vector<OrderId>> m_bids;
    std::map<Price, std::vector<OrderId>> m_asks;
    std::unordered_map<OrderId, Order> m_idToOrder;
    OrderId m_nextId = 1;
    bool validateOrder(Quantity quantity, Price price, Side side) const; 
    void matchOrder(Order& order, std::vector<Trade>& trades);
public:
    bool cancelOrder(OrderId id);
    std::optional<SubmissionResult> addOrder(Quantity quantity, Price price, Side side);
    void printOrderBook() const;
};

#endif