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

    OrderId Id{};
    ASSERT_TRUE(book.addOrder(10, 100, Side::Sell, Id));

    ASSERT_FALSE(book.cancelOrder(2));
}