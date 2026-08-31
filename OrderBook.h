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
    struct RestingOrder
    {
        Order order;
        RestingOrder* prev;
        RestingOrder* next;
        RestingOrder(const Order& o) : order(o), prev(nullptr), next(nullptr) {}
    };
    struct PriceLevel
    {
        RestingOrder* head;
        RestingOrder* tail;
        Quantity totalQuantity;
        PriceLevel() : head(nullptr), tail(nullptr), totalQuantity(0) {}
    };

    std::unordered_map<OrderId, RestingOrder> m_idToOrder;
    std::map<Price, PriceLevel> m_bids;
    std::map<Price, PriceLevel> m_asks;
    OrderId m_nextId = 1;

    void appendToPriceLevel(PriceLevel& priceLevel, RestingOrder& order);
    void removeFromPriceLevel(PriceLevel& priceLevel, RestingOrder& order);
    bool validateOrder(Quantity quantity, Price price, Side side) const; 
    void matchOrder(Order& order, std::vector<Trade>& trades);
public:
    //CANNOT COPY, MOVE, ASSIGN!!
    bool cancelOrder(OrderId id);
    std::optional<SubmissionResult> addOrder(Quantity quantity, Price price, Side side);
    void printOrderBook() const;
    std::optional<Price> bestBid() const;
    std::optional<Price> bestAsk() const;
    std::optional<Order> findOrder(OrderId id) const;
};

#endif