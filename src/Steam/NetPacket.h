#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace NetPacket {
enum class Layout { Legacy, Steam1792618285 };

constexpr Layout ForSteamClientHash(std::string_view) noexcept {
    return Layout::Legacy;
}

// Operate on the original packet address; never pass an adjusted view to Steam.
class View {
public:
    View(void* packet, Layout layout) noexcept
        : packet_(static_cast<std::byte*>(packet)),
          dataOffset_(layout == Layout::Steam1792618285 ? 0x10u : 0x08u) {}

    std::uint8_t* Data() const noexcept {
        std::uint8_t* value;
        std::memcpy(&value, packet_ + dataOffset_, sizeof(value));
        return value;
    }
    std::uint32_t Size() const noexcept {
        std::uint32_t value;
        std::memcpy(&value, packet_ + dataOffset_ + sizeof(void*), sizeof(value));
        return value;
    }
    void SetData(std::uint8_t* value) const noexcept {
        std::memcpy(packet_ + dataOffset_, &value, sizeof(value));
    }
    void SetSize(std::uint32_t value) const noexcept {
        std::memcpy(packet_ + dataOffset_ + sizeof(void*), &value, sizeof(value));
    }
private:
    std::byte* packet_;
    std::size_t dataOffset_;
};
} // namespace NetPacket
