#include "OrderBook.h"
#include <gtest/gtest.h>

TEST(OrderBookValidation, InvalidOrderDoesNotConsumeId)
{
    OrderBook book;
    
    OrderId rejectedId{};
    EXPECT_FALSE(book.addOrder(0, 100, Side::Buy, rejectedId));

    OrderId acceptedId{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Buy, acceptedId));

    EXPECT_EQ(acceptedId, 1);
}

TEST(OrderBookValidation, NegativePriceIsRejected)
{
    OrderBook book;

    OrderId Id{};
    EXPECT_FALSE(book.addOrder(10, -100, Side::Buy, Id));
}

TEST(OrderBookValidation, ZeroPriceIsRejected)
{
    OrderBook book;

    OrderId Id{};
    EXPECT_FALSE(book.addOrder(10, 0, Side::Buy, Id));
}

TEST(OrderBookValidation, NegativeQuantityIsRejected)
{
    OrderBook book;

    OrderId Id{};
    EXPECT_FALSE(book.addOrder(-10, 100, Side::Buy, Id));
}

TEST(OrderBookValidation, AddValidBuyRestingOrder)
{
    OrderBook book;

    OrderId Id{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Buy, Id));

    EXPECT_EQ(Id, 1);

    ASSERT_TRUE(book.cancelOrder(1));
}

TEST(OrderBookValidation, AddValidSellRestingOrder)
{
    OrderBook book;

    OrderId Id{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Sell, Id));

    EXPECT_EQ(Id, 1);

    ASSERT_TRUE(book.cancelOrder(1));
}

TEST(OrderBookValidation, RestingOrderCanBeCancelled)
{
    OrderBook book;

    OrderId Id{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Sell, Id));

    ASSERT_TRUE(book.cancelOrder(1));
}

TEST(OrderBookValidation, SameIdCancellation)
{
    OrderBook book;

    OrderId Id{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Sell, Id));

    ASSERT_TRUE(book.cancelOrder(1));

    ASSERT_FALSE(book.cancelOrder(1));
}

TEST(OrderBookValidation, InvalidIdCancellation)
{
    OrderBook book;

    OrderId id{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Sell, id));

    ASSERT_FALSE(book.cancelOrder(2));
}

TEST(OrderBookMatching, BuyExactFill)
{
    OrderBook book;

    OrderId id{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Sell, id));

    OrderId NewId{};
    ASSERT_TRUE(book.addOrder(10, 101, Side::Buy, NewId));

    ASSERT_FALSE(book.cancelOrder(id));
    ASSERT_FALSE(book.cancelOrder(NewId));
}

TEST(OrderBookMatching, SellExactFill)
{
    OrderBook book;

    OrderId id{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Buy, id));

    OrderId NewId{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Sell, NewId));

    ASSERT_FALSE(book.cancelOrder(id));
    ASSERT_FALSE(book.cancelOrder(NewId));
}

TEST(OrderBookMatching, BuyPartialFill)
{
    OrderBook book;

    OrderId id{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Sell, id));

    OrderId NewId{};
    ASSERT_TRUE(book.addOrder(30, 101, Side::Buy, NewId));

    ASSERT_FALSE(book.cancelOrder(id));

    //check if correct quantity goes into book (20 remain)
    OrderId restingSellId{};
    ASSERT_TRUE(book.addOrder(20, 101, Side::Sell, restingSellId));
    ASSERT_FALSE(book.cancelOrder(restingSellId));
    ASSERT_FALSE(book.cancelOrder(NewId));
}

TEST(OrderBookMatching, SellPartialFill)
{
    OrderBook book;

    OrderId id{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Buy, id));

    OrderId NewId{};
    ASSERT_TRUE(book.addOrder(30, 99, Side::Sell, NewId));

    ASSERT_FALSE(book.cancelOrder(id));

    //check if correct quantity goes into book (20 remain)
    OrderId restingBuyId{};
    ASSERT_TRUE(book.addOrder(20, 99, Side::Buy, restingBuyId));
    ASSERT_FALSE(book.cancelOrder(restingBuyId));
    ASSERT_FALSE(book.cancelOrder(NewId));
}

TEST(OrderBookMatching, BuyPartiallyFillsRestingSell)
{
    OrderBook book;

    OrderId id{};
    ASSERT_TRUE(book.addOrder(50, 100, Side::Sell, id));

    OrderId NewId{};
    ASSERT_TRUE(book.addOrder(30, 101, Side::Buy, NewId));

    ASSERT_FALSE(book.cancelOrder(NewId));

    OrderId probeBuyId{};
    ASSERT_TRUE(book.addOrder(20, 100, Side::Buy, probeBuyId));
    
    ASSERT_FALSE(book.cancelOrder(id));
    ASSERT_FALSE(book.cancelOrder(probeBuyId));
}

TEST(OrderBookMatching, SellPartiallyFillsRestingBuy)
{
    OrderBook book;

    OrderId id{};
    ASSERT_TRUE(book.addOrder(50, 100, Side::Buy, id));

    OrderId NewId{};
    ASSERT_TRUE(book.addOrder(30, 99, Side::Sell, NewId));

    ASSERT_FALSE(book.cancelOrder(NewId));

    OrderId probeSellId{};
    ASSERT_TRUE(book.addOrder(20, 100, Side::Sell, probeSellId));

    ASSERT_FALSE(book.cancelOrder(id));
    ASSERT_FALSE(book.cancelOrder(probeSellId));
}
