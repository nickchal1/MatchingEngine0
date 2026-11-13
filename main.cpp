#include "OrderBook.h"

#include <iostream>

int main()
{
    //no cross
    OrderBook ladder;

    ladder.addOrder(30, 10500, Side::Sell);
    ladder.addOrder(15, 10400, Side::Sell);
    ladder.addOrder(25, 10400, Side::Sell);
    ladder.addOrder(40, 10300, Side::Sell);
    ladder.addOrder(10, 10200, Side::Sell);

    ladder.addOrder(20, 10100, Side::Buy);
    ladder.addOrder(35, 10000, Side::Buy);
    ladder.addOrder(5,  10000, Side::Buy);
    ladder.addOrder(50000, 9900,  Side::Buy);
    ladder.addOrder(12, 9800,  Side::Buy);

    std::cout << "\nOrder book:\n";
    ladder.printOrderBook();

    return 0;
}
