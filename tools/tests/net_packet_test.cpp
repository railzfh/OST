#include "Steam/NetPacket.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace {
constexpr std::string_view kCurrentClient =
    "253e0a11a4415b38c78a42f7bdd6ecc426455f06c1ba6453a48090df3142dbbc";

template<class T>
void Store(std::array<std::byte, 48>& bytes, std::size_t offset, T value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

bool CheckPacket(std::string_view hash, std::size_t dataOffset,
                 std::size_t sizeOffset, const char* label) {
    std::array<std::byte, 48> raw;
    raw.fill(std::byte{0xA5});
    std::uint8_t originalData[]{0x01, 0x00, 0x00, 0x80, 0, 0, 0, 0};
    std::uint8_t replacement[]{0x02, 0x00, 0x00, 0x80};
    Store(raw, dataOffset, static_cast<std::uint8_t*>(originalData));
    Store(raw, sizeOffset, std::uint32_t{8});
    const auto original = raw;

    NetPacket::View packet(raw.data(), NetPacket::ForSteamClientHash(hash));
    if (packet.Data() != originalData || packet.Size() != 8) {
        std::printf("FAIL: %s packet payload read at wrong offsets\n", label);
        return false;
    }

    packet.SetData(replacement);
    packet.SetSize(4);
    auto expected = original;
    Store(expected, dataOffset, static_cast<std::uint8_t*>(replacement));
    Store(expected, sizeOffset, std::uint32_t{4});
    if (raw != expected || packet.Data() != replacement || packet.Size() != 4) {
        std::printf("FAIL: %s replacement corrupted adjacent fields\n", label);
        return false;
    }

    // Both carrier injection paths must restore the original packet fields.
    packet.SetData(originalData);
    packet.SetSize(8);
    if (raw != original) {
        std::printf("FAIL: %s carrier fields were not restored\n", label);
        return false;
    }
    return true;
}
} // namespace

int main() {
    static_assert(sizeof(void*) == 8, "Steam client packets are x64");
    bool ok = true;
    ok &= CheckPacket("older-client-hash", 0x08, 0x10, "legacy");
    ok &= CheckPacket(kCurrentClient, 0x10, 0x18, "1792618285");
    ok &= CheckPacket("", 0x08, 0x10, "missing hash");
    // A nearby hash must not accidentally select an unverified ABI.
    ok &= CheckPacket("253e0a11a4415b38c78a42f7bdd6ecc426455f06c1ba6453a48090df3142dbbd",
                      0x08, 0x10, "different client");
    if (!ok) return 1;
    std::puts("PASS: legacy/current payload access, adjacent fields and carrier restoration");
    return 0;
}
