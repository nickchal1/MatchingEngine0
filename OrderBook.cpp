#include "OrderBook.h"

#include <algorithm>
#include <iostream>

void OrderBook::appendToPriceLevel(PriceLevel& priceLevel, RestingOrder& order) {
    if (!priceLevel.tail) {
        priceLevel.head = &order;
        priceLevel.tail = &order;
    }
    //not empty LL
    else {
        priceLevel.tail->next = &order;
        order.prev = priceLevel.tail;
        priceLevel.tail = &order;
    }
    priceLevel.totalQuantity += order.order.quantity;
}

void OrderBook::removeFromPriceLevel(PriceLevel& priceLevel, RestingOrder& order) {
    //one item
    if(priceLevel.head == priceLevel.tail) {
        priceLevel.tail = nullptr;
        priceLevel.head = nullptr;
    }
    //head
    else if(&order == priceLevel.head) {
        order.next->prev = nullptr;
        priceLevel.head = order.next;
    }
    //tail
    else if (&order == priceLevel.tail) {
        order.prev->next = nullptr;
        priceLevel.tail = order.prev;
    }
    //middle
    else {
        order.prev->next = order.next;
        order.next->prev = order.prev;
    }
    order.next = nullptr;
    order.prev = nullptr;
    priceLevel.totalQuantity -= order.order.quantity;
}

bool OrderBook::validateOrder(Quantity quantity, Price price, Side side) const {
    if (quantity <= 0 || price <= 0) {
        return false;
    }

    if (side != Side::Buy && side != Side::Sell) {
        return false;
    }
    return true;
}

void OrderBook::matchOrder(Order& order, std::vector<Trade>& trades) {

    if (order.side == Side::Buy) {
        while (!m_asks.empty() && m_asks.begin()->first <= order.price) {

            PriceLevel& priceLev = m_asks.begin()->second;
            //process time prio order
            OrderId currId = priceLev.head->order.id;
            RestingOrder* restingOr = priceLev.head;
            Order& currOrder = priceLev.head->order;

            Trade trade;
            trade.makerId = currId;
            trade.takerId = order.id;
            trade.executionQuantity = std::min(order.quantity, currOrder.quantity);
            trade.executionPrice = currOrder.price;
            trade.takerSide = order.side;
            trades.push_back(trade);
            
            if (order.quantity < currOrder.quantity) {
                currOrder.quantity -= order.quantity;
                priceLev.totalQuantity -= order.quantity;
                order.quantity = 0;
                //fully filled
                return;
            }
            else {
                //delete curr order
                order.quantity -= currOrder.quantity;
                removeFromPriceLevel(priceLev, *restingOr);
                if (!priceLev.head) {
                    m_asks.erase(m_asks.begin()->first);
                }
                //price level empty
                m_idToOrder.erase(currId);
                if (order.quantity == 0) {
                    return;
                }
                continue;
            }
        }
    }
    //Side Sell
    else {
        while (!m_bids.empty() && m_bids.rbegin()->first >= order.price) {

            PriceLevel& priceLev = m_bids.rbegin()->second;
            //process time prio order
            OrderId currId = priceLev.head->order.id;
            RestingOrder* restingOr = priceLev.head;
            Order& currOrder = priceLev.head->order;

            Trade trade;
            trade.makerId = currId;
            trade.takerId = order.id;
            trade.executionQuantity = std::min(order.quantity, currOrder.quantity);
            trade.executionPrice = currOrder.price;
            trade.takerSide = order.side;
            trades.push_back(trade);
            
            if (order.quantity < currOrder.quantity) {
                currOrder.quantity -= order.quantity;
                priceLev.totalQuantity -= order.quantity;
                order.quantity = 0;
                //fully filled
                return;
            }
            else {
                //delete curr order
                order.quantity -= currOrder.quantity;
                removeFromPriceLevel(priceLev, *restingOr);
                //price level empty
                if (!priceLev.head) {
                    m_bids.erase(m_bids.rbegin()->first);
                }
                m_idToOrder.erase(currId);
                if (order.quantity == 0) {
                    return;
                }
                continue;
            }
        }
    }
}

std::optional<SubmissionResult> OrderBook::addOrder(Quantity quantity, Price price, Side side) {
    
    //checks
    if (!validateOrder(quantity, price, side)) {
        return std::nullopt;
    }

    //create order + subRes
    Order order = {m_nextId, quantity, price, side};
    //id and empty trades vec
    SubmissionResult result = {m_nextId, {}};

    //match Buy order
    matchOrder(order, result.trades);

    if (order.quantity != 0) {
        
        //construct resting + add to map
        auto [it, inserted] = m_idToOrder.try_emplace(m_nextId, order);

        //add to price level (IF partial fill)
        if (side == Side::Buy) {
            //create level
            auto [level_it, level_inserted] = m_bids.try_emplace(price);
            appendToPriceLevel(level_it->second, it->second);
        }
        else {
            auto [level_it, level_inserted] = m_asks.try_emplace(price);
            appendToPriceLevel(level_it->second, it->second);
        }
    }
    m_nextId++;
    return result;
}

bool OrderBook::cancelOrder(OrderId id) {
    
    auto orderIt = m_idToOrder.find(id);

    if (orderIt == m_idToOrder.end()) {
        return false;
    }
    Side side = orderIt->second.order.side;
    Price price = orderIt->second.order.price;

    //map side 
    auto& sideMap = side == Side::Sell ? m_asks : m_bids;

    //find in side maps
    auto vectorOrderIt = sideMap.find(price);
    if (vectorOrderIt != sideMap.end()) {
        PriceLevel& priceLev = vectorOrderIt->second;
        if (!priceLev.head) {
            return false;
        }
        else {
            if (orderIt != m_idToOrder.end()) {
                //here vector is found in the correct price
                removeFromPriceLevel(priceLev, orderIt->second);
                if (!priceLev.head) {
                    sideMap.erase(price);
                }
                //erase order from idOrder map
                m_idToOrder.erase(orderIt);
                return true;
            }
            return false;
        }
    }
    else {
        return false;
    }
}

//TODO: fix print
// void OrderBook::printOrderBook() const {

//     std::cout<<"Bids"<<'\n';

//     auto mapIt = m_bids.rbegin();
//     while (mapIt != m_bids.rend()) {
//         //price
//         std::cout<<mapIt->first<<'\n';
//         auto& priceVector = mapIt->second;
//         for (OrderId currId : priceVector) {
//             //id
//             std::cout<<currId<<'\n';
//             auto orderIt = m_idToOrder.find(currId);
//             //quantity
//             std::cout<<orderIt->second.quantity<<'\n';
//         }
//         mapIt++;
//     }

//     std::cout<<"Asks"<<'\n';

//     auto askMapIt = m_asks.begin();
//     while (askMapIt != m_asks.end()) {
//         //price
//         std::cout<<askMapIt->first<<'\n';
//         auto& priceVector = askMapIt->second;
//         for (OrderId currId : priceVector) {
//             //id
//             std::cout<<currId<<'\n';
//             auto orderIt = m_idToOrder.find(currId);
//             //quantity
//             std::cout<<orderIt->second.quantity<<'\n';
//         }
//         askMapIt++;
//     }
// }

std::optional<Price> OrderBook::bestBid() const {

    if (m_bids.empty()) {
        return std::nullopt;
    }
    return m_bids.rbegin()->first;
}

std::optional<Price> OrderBook::bestAsk() const {

    if (m_asks.empty()) {
        return std::nullopt;
    }
    return m_asks.begin()->first;
}

std::optional<Order> OrderBook::findOrder(OrderId id) const {
    auto orderIt = m_idToOrder.find(id);

    if(orderIt == m_idToOrder.end()) {
        return std::nullopt;
    }
    return orderIt->second.order;
}