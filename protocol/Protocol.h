#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "Order.h"
#include <vector>
#include <optional>


using ClientOrderId = std::uint64_t;
constexpr std::size_t NewOrderSize = 22;

enum class MessageType : std::uint8_t
{
    NewOrder = 1,
    Cancel = 2,
    Ack = 3,
    Reject = 4,
    Fill = 5,
};

enum class AckType : std::uint8_t
{
    Accepted = 1,
    Cancelled = 2,
};

struct NewOrder
{
    ClientOrderId clientId;
    Side side;
    Price price;
    Quantity quantity;
};

struct Cancel
{
    ClientOrderId clientId;
};

struct Ack
{
    ClientOrderId clientId;
    OrderId exchangeOrderId;
    AckType ackType;
};

//need to imp Reject + Fill

void encode(const NewOrder& newOrder, std::vector<std::uint8_t>& buffer);
std::optional<NewOrder> decode(const std::vector<std::uint8_t>& buffer);

template<typename T>
void frameEncode(const T& msg, std::vector<std::uint8_t>& buffer) {

    std::size_t start = buffer.size();
    buffer.push_back(0);

    encode(msg, buffer);
    
    std::size_t size = buffer.size() - start - 1;
    buffer[start] = static_cast<std::uint8_t>(size);
}

#endif