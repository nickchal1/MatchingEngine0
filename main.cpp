#include "OrderBook.h"

#include <iostream>

int main()
{
    OrderBook book;

    // OrderId bid100First{};
    // if (!book.addOrder(100, 10000, Side::Buy, bid100First)) {
    //     std::cerr << "Failed to add bid100First\n";
    //     return 1;
    // }
    // book.printOrderBook();

    // OrderId bid101{};
    // if (!book.addOrder(50, 10100, Side::Buy, bid101)) {
    //     std::cerr << "Failed to add bid101\n";
    //     return 1;
    // }
    // book.printOrderBook();

    // OrderId bid100Second{};
    // if (!book.addOrder(75, 10000, Side::Buy, bid100Second)) {
    //     std::cerr << "Failed to add bid100Second\n";
    //     return 1;
    // }
    // book.printOrderBook();

    // OrderId bid100Third{};
    // if (!book.addOrder(25, 10000, Side::Buy, bid100Third)) {
    //     std::cerr << "Failed to add bid100Third\n";
    //     return 1;
    // }
    // book.printOrderBook();

    // OrderId bid99{};
    // if (!book.addOrder(40, 9900, Side::Buy, bid99)) {
    //     std::cerr << "Failed to add bid99\n";
    //     return 1;
    // }
    // book.printOrderBook();

    // OrderId ask102First{};
    // if (!book.addOrder(60, 10200, Side::Sell, ask102First)) {
    //     std::cerr << "Failed to add ask102First\n";
    //     return 1;
    // }
    // book.printOrderBook();

    // OrderId ask103{};
    // if (!book.addOrder(40, 10300, Side::Sell, ask103)) {
    //     std::cerr << "Failed to add ask103\n";
    //     return 1;
    // }
    // book.printOrderBook();

    // OrderId ask102Second{};
    // if (!book.addOrder(90, 10200, Side::Sell, ask102Second)) {
    //     std::cerr << "Failed to add ask102Second\n";
    //     return 1;
    // }
    // book.printOrderBook();

    // OrderId ask104{};
    // if (!book.addOrder(30, 10400, Side::Sell, ask104)) {
    //     std::cerr << "Failed to add ask104\n";
    //     return 1;
    // }
    // book.printOrderBook();

    // if (!book.cancelOrder(bid100Second)) {
    //     std::cerr << "Failed to cancel bid100Second\n";
    //     return 1;
    // }
    // book.printOrderBook();

    // if (!book.cancelOrder(ask102First)) {
    //     std::cerr << "Failed to cancel ask102First\n";
    //     return 1;
    // }
    book.printOrderBook();

    return 0;
}
