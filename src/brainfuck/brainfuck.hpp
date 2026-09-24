#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace hypertension::bf {

struct RunResult {
    std::string output;
    std::string error; // empty = success
};

[[nodiscard]] auto run(std::string_view program, std::span<const uint8_t> input) -> RunResult;

} // namespace hypertension::bf
