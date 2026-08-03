#include "OrderBook.h"

#include <algorithm>
#include <iostream>

bool OrderBook::addOrder(Quantity quantity, Price price, Side side, OrderId& assignedId) {
    
    //checks
    if (quantity <= 0 || price <= 0) {
        return false;
    }

    if (side != Side::Buy && side != Side::Sell) {
        return false;
    }

    Order order = {m_nextId, quantity, price, side};
    assignedId = m_nextId;

    //add to price map
    if (side == Side::Buy) {
        m_bids[price].push_back(m_nextId);
    }
    else {
        m_asks[price].push_back(m_nextId);
    }

    m_idToOrder.insert({m_nextId, order});
    m_nextId++;
    return true;
}

bool OrderBook::cancelOrder(OrderId id) {
    
    auto orderIt = m_idToOrder.find(id);

    if (orderIt == m_idToOrder.end()) {
        return false;
    }
    Side side = orderIt->second.side;
    Price price = orderIt->second.price;

    //map side 
    auto& sideMap = side == Side::Sell ? m_asks : m_bids;

    //find in side maps
    auto vectorOrderIt = sideMap.find(price);
    if (vectorOrderIt != sideMap.end()) {
        auto& priceVector = vectorOrderIt->second;
        if (priceVector.empty()) {
            return false;
        }
        else {
            auto findIt = std::find(priceVector.begin(), priceVector.end(), id);
            if (findIt != priceVector.end()) {
                //here vector is found in the correct price
                priceVector.erase(findIt);
                if (priceVector.empty()) {
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

void OrderBook::printOrderBook() const {

    std::cout<<"Bids"<<'\n';

    auto mapIt = m_bids.rbegin();
    while (mapIt != m_bids.rend()) {
        //price
        std::cout<<mapIt->first<<'\n';
        auto& priceVector = mapIt->second;
        for (OrderId currId : priceVector) {
            //id
            std::cout<<currId<<'\n';
            auto orderIt = m_idToOrder.find(currId);
            //quantity
            std::cout<<orderIt->second.quantity<<'\n';
        }
        mapIt++;
    }

    std::cout<<"Asks"<<'\n';

    auto askMapIt = m_asks.begin();
    while (askMapIt != m_asks.end()) {
        //price
        std::cout<<askMapIt->first<<'\n';
        auto& priceVector = askMapIt->second;
        for (OrderId currId : priceVector) {
            //id
            std::cout<<currId<<'\n';
            auto orderIt = m_idToOrder.find(currId);
            //quantity
            std::cout<<orderIt->second.quantity<<'\n';
        }
        askMapIt++;
    }
}