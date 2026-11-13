#include "Protocol.h"
#include <gtest/gtest.h>

void roundTrip(const NewOrder& order) {
    std::vector<std::uint8_t> buff;
    encode(order, buff);
    auto decoded = decode(buff);

    ASSERT_TRUE(decoded);
    EXPECT_EQ(order.clientId, decoded->clientId);
    EXPECT_EQ(order.side, decoded->side);
    EXPECT_EQ(order.price, decoded->price);
    EXPECT_EQ(order.quantity, decoded->quantity);
}

TEST(ProtocolRoundTrip, Buy)
{
    NewOrder order{.clientId = 5, .side = Side::Buy, .price = 100, .quantity = 200};
    roundTrip(order);
}

TEST(ProtocolRoundTrip, Sell)
{
    NewOrder order{.clientId = 5, .side = Side::Sell, .price = 100, .quantity = 200};
    roundTrip(order);
}

TEST(ProtocolRoundTrip, MaxVals)
{
    //quant + price is INT!!
    NewOrder order{.clientId = UINT64_MAX, .side = Side::Sell, .price = INT32_MAX, .quantity = INT64_MAX};
    roundTrip(order);
}

TEST(ProtocolRoundTrip, MinValsIdOne)
{
    //quant + price is INT!!
    NewOrder order{.clientId = 1, .side = Side::Sell, .price = INT32_MIN, .quantity = INT64_MIN};
    roundTrip(order);
}

TEST(ProtocolDecode, BytesMustBe22)
{
    NewOrder order{.clientId = 5, .side = Side::Buy, .price = 100, .quantity = 200};
    std::vector<std::uint8_t> buff;
    encode(order, buff);
    buff.push_back(0xFF);
    auto decoded = decode(buff);

    //nullopt
    ASSERT_FALSE(decoded);
}

TEST(ProtocolDecode, RejectLess22)
{
    NewOrder order{.clientId = 5, .side = Side::Buy, .price = 100, .quantity = 200};
    std::vector<std::uint8_t> buff;
    encode(order, buff);
    buff.pop_back();
    auto decoded = decode(buff);

    //nullopt
    ASSERT_FALSE(decoded);
}

TEST(ProtocolDecode, RejectWrongType)
{
    NewOrder order{.clientId = 5, .side = Side::Buy, .price = 100, .quantity = 200};
    std::vector<std::uint8_t> buff;
    encode(order, buff);
    //make it "cancel" type
    buff[0] = 0x02;
    auto decoded = decode(buff);

    //nullopt
    ASSERT_FALSE(decoded);
}


