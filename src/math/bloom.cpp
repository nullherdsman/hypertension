#include "bloom.hpp"
#include "../brainfuck/brainfuck.hpp"

#include <cstdint>

namespace hypertension::math {

// Bloom filter: m=16 bits, k=2, n=7 elements.
//
// h1(x) = MEMBERSHIP_BF(x & 0xFF) % 16    (BF computes (3x+7) mod 256)
// h2(x) = (x & 0xFF) % 16
//
// Precomputed bits for {7,13,21,42,64,128,256}:
//   h1: {12,14,6,5,7}  h2: {7,13,5,10,0}  union: {0,5,6,7,10,12,13,14}
constexpr uint16_t BLOOM_BITS = 0x74E1u;

auto bloom_check(std::string_view bf_program, int value, std::string& error)
    -> std::optional<bool>
{
    // Truncate to 8 bits. Values outside [0, 255] cannot be members of the
    // dataset (all seven elements fit in a byte), but the caller may pass
    // arbitrary signed integers. Masking with 0xFF ensures the hash function
    // receives a well-defined input byte regardless of signedness or width.
    uint8_t input_byte = static_cast<uint8_t>(value & 0xFF);
    auto result = bf::run(bf_program, std::span<const uint8_t>{&input_byte, 1});

    if (!result.error.empty()) { error = result.error; return std::nullopt; }
    if (result.output.empty()) { error = "BF program produced no output"; return std::nullopt; }

    int h1 = static_cast<uint8_t>(result.output[0]) % 16;
    int h2 = input_byte % 16;

    return static_cast<bool>(((BLOOM_BITS >> h1) & 1) && ((BLOOM_BITS >> h2) & 1));
}

auto verify_membership(int value, std::string& error) -> std::optional<bool> {
    return bloom_check(MEMBERSHIP_BF, value, error);
}

} // namespace hypertension::math
