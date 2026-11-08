#include "Protocol.h"

#include <vector>
#include <optional>

void writeU8(std::vector<std::uint8_t>& buffer, std::uint8_t val) {
    buffer.push_back(val);
}

void writeU32(std::vector<std::uint8_t>& buffer, std::uint32_t val) {
    buffer.push_back((val >> 24) & 0xFF);
    buffer.push_back((val >> 16) & 0xFF);
    buffer.push_back((val >> 8) & 0xFF);
    buffer.push_back(val & 0xFF);
}

void writeU64(std::vector<std::uint8_t>& buffer, std::uint64_t val) {
    for (int i = 56; i >= 0; i -= 8) {
        buffer.push_back((val >> i) & 0xFF);
    }
}

std::uint8_t readU8(const std::vector<std::uint8_t>& buffer, std::size_t& offset) {
    std::uint8_t val = buffer[offset];
    offset++;
    return val;
}

std::uint32_t readU32(const std::vector<std::uint8_t>& buffer, std::size_t& offset) {
    std::uint32_t val = 0;
    for (int i = 0; i < 4; ++i) {
        //shift + OR next byte
        val = (val << 8) | readU8(buffer, offset);
    }
    return val;
}

std::uint64_t readU64(const std::vector<std::uint8_t>& buffer, std::size_t& offset) {
    std::uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val = (val << 8) | readU8(buffer, offset);
    }
    return val;
}

void encode(const NewOrder& newOrder, std::vector<std::uint8_t>& buffer) {
    writeU8(buffer, static_cast<uint8_t>(MessageType::NewOrder));
    writeU64(buffer, newOrder.clientId);
    writeU8(buffer, static_cast<uint8_t>(newOrder.side));
    writeU32(buffer, newOrder.price);
    writeU64(buffer, newOrder.quantity);
}

std::optional<NewOrder> decode(const std::vector<std::uint8_t>& buffer) {
    //not full order
    if (buffer.size() < 22) {
        return std::nullopt;
    }
    std::size_t offset = 0;

    //type, id, side, price, quantity
    std::uint8_t type = readU8(buffer, offset);
    if (type != static_cast<uint8_t>(MessageType::NewOrder)) {
        return std::nullopt;
    }
    ClientOrderId id = readU64(buffer, offset);
    Side side = static_cast<Side>(readU8(buffer, offset));
    Price price = readU32(buffer, offset);
    Quantity quantity = readU64(buffer, offset);

    NewOrder order{id, quantity, price, side};
    return order;
}