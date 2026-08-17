#include "OrderBook.h"
#include <gtest/gtest.h>

TEST(OrderBookValidation, InvalidOrderDoesNotConsumeId)
{
    OrderBook book;
    
    auto rejectedResult = book.addOrder(0, 100, Side::Buy);
    EXPECT_FALSE(rejectedResult);

    auto acceptedResult = book.addOrder(10, 100, Side::Buy);
    ASSERT_TRUE(acceptedResult);

    EXPECT_EQ(acceptedResult->orderId, 1);
}

TEST(OrderBookValidation, NegativePriceIsRejected)
{
    OrderBook book;
    const auto result = book.addOrder(10, -100, Side::Buy);
    EXPECT_FALSE(result);
}

TEST(OrderBookValidation, ZeroPriceIsRejected)
{
    OrderBook book;
    const auto result = book.addOrder(10, 0, Side::Buy);
    EXPECT_FALSE(result);
}

TEST(OrderBookValidation, NegativeQuantityIsRejected)
{
    OrderBook book;
    const auto result = book.addOrder(-10, 100, Side::Buy);
    EXPECT_FALSE(result);
}

TEST(OrderBookValidation, InvalidSideIsRejected)
{
    OrderBook book;
    const Side invalidSide = static_cast<Side>(2);
    const auto result = book.addOrder(10, 100, invalidSide);
    EXPECT_FALSE(result);
}

TEST(OrderBookValidation, MinValidPriceQuantityValues)
{
    OrderBook book;
    const auto result = book.addOrder(1, 1, Side::Buy);
    EXPECT_TRUE(result);
}

TEST(OrderBookValidation, MonotonicIdGenerationResting)
{
    OrderBook book;
    for (size_t i = 0; i < 10; ++i) {
        const auto result = book.addOrder(10, 100, Side::Buy);
        ASSERT_TRUE(result);
        ASSERT_EQ(result->orderId, i + 1);
    }
}

TEST(OrderBookValidation, MonotonicIdGenerationFilled)
{
    OrderBook book;
    size_t counter = 0;
    for (counter; counter < 10; ++counter) {
        const auto result = book.addOrder(10, 100, Side::Buy);
    }
    for (counter; counter < 20; ++counter) {
        const auto result = book.addOrder(10, 100, Side::Sell);
        ASSERT_TRUE(result);
        ASSERT_EQ(result->orderId, counter + 1);
    }
}

TEST(OrderBookValidation, EmptyBookBestBidAskFound) 
{
    OrderBook book;
    EXPECT_FALSE(book.findOrder(1));
    EXPECT_FALSE(book.bestAsk());
    EXPECT_FALSE(book.bestBid());
}

TEST(OrderBookValidation, AddValidBuyRestingOrder)
{
    OrderBook book;
    const auto result = book.addOrder(10, 100, Side::Buy);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->orderId, 1);
    EXPECT_EQ(result->trades.size(), 0);
    EXPECT_TRUE(book.findOrder(result->orderId));
    EXPECT_EQ(book.bestBid(), 100);
}

TEST(OrderBookValidation, AddValidSellRestingOrder)
{
    OrderBook book;
    const auto result = book.addOrder(10, 100, Side::Sell);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->orderId, 1);
    EXPECT_EQ(result->trades.size(), 0);
    EXPECT_TRUE(book.findOrder(result->orderId));
    EXPECT_EQ(book.bestAsk(), 100);
}

TEST(OrderBookValidation, MultipleBidAskPriceLevelsAndNoCross)
{
    OrderBook book;
    for (size_t i = 0; i < 10; ++i) {
        book.addOrder(10, 100 + i, Side::Buy);
        book.addOrder(10, 110 + i, Side::Sell);
    }
    EXPECT_EQ(book.bestBid(), 109);
    EXPECT_EQ(book.bestAsk(), 110);
}

TEST(OrderBookValidation, InvalidIdCancellation)
{
    OrderBook book;
    ASSERT_TRUE(book.addOrder(10, 100, Side::Sell));
    ASSERT_FALSE(book.cancelOrder(2));
}

TEST(OrderBookValidation, RestingBuyOrderCanBeCancelled)
{
    OrderBook book;
    const auto result = book.addOrder(10, 100, Side::Buy);
    ASSERT_TRUE(result);
    ASSERT_TRUE(book.cancelOrder(result->orderId));
}

TEST(OrderBookValidation, RestingSellOrderCanBeCancelled)
{
    OrderBook book;
    const auto result = book.addOrder(10, 100, Side::Sell);
    ASSERT_TRUE(result);
    ASSERT_TRUE(book.cancelOrder(result->orderId));
}

TEST(OrderBookValidation, SameIdCancellation)
{
    OrderBook book;
    const auto result = book.addOrder(10, 100, Side::Sell);
    ASSERT_TRUE(result);
    ASSERT_TRUE(book.cancelOrder(result->orderId));
    ASSERT_FALSE(book.cancelOrder(result->orderId));
}

TEST(OrderBookValidation, FIFOPreservedAfterCancellation)
{
    OrderBook book;
    const auto result1 = book.addOrder(30, 100, Side::Sell);
    const auto result2 = book.addOrder(20, 100, Side::Sell);
    const auto result3 = book.addOrder(10, 100, Side::Sell);

    ASSERT_TRUE(book.cancelOrder(result2->orderId));
    ASSERT_FALSE(book.findOrder(result2->orderId));
    ASSERT_TRUE(book.findOrder(result1->orderId));
    ASSERT_TRUE(book.findOrder(result3->orderId));
    EXPECT_EQ(book.bestAsk(), 100);

    const auto match1 = book.addOrder(30, 100, Side::Buy);
    EXPECT_EQ(match1->trades[0].makerId, result1->orderId);
}

TEST(OrderBookValidation, CancelOnlyOrderAtPriceUpdatesBestBid)
{
    OrderBook book;
    const auto result1 = book.addOrder(30, 110, Side::Buy);
    const auto result2 = book.addOrder(20, 100, Side::Buy);

    ASSERT_TRUE(book.cancelOrder(result1->orderId));
    EXPECT_EQ(book.bestBid(), 100);
}


TEST(OrderBookMatching, SellExactFill)
{
    OrderBook book;
    const auto resultBuy = book.addOrder(20, 110, Side::Buy);
    const auto resultSell = book.addOrder(20, 110, Side::Sell);

    EXPECT_EQ(resultSell->trades.size(), 1);
    EXPECT_EQ(resultBuy->orderId, resultSell->trades[0].makerId);
    EXPECT_EQ(resultSell->trades[0].executionQuantity, 20);
}

TEST(OrderBookMatching, BuyExactFill)
{
    OrderBook book;
    const auto resultSell = book.addOrder(20, 110, Side::Sell);
    const auto resultBuy = book.addOrder(20, 110, Side::Buy);

    EXPECT_EQ(resultBuy->trades.size(), 1);
    EXPECT_EQ(resultSell->orderId, resultBuy->trades[0].makerId);
    EXPECT_EQ(resultBuy->trades[0].executionQuantity, 20);
}

