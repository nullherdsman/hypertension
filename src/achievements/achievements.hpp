#pragma once

#include <string_view>

namespace hypertension::achievements {

struct Achievement {
    std::string_view id;
    std::string_view title;
    std::string_view description;
};

constexpr Achievement MATHEMATICALLY_CORRECT{
    "mathematically_correct",
    "MATHEMATICALLY CORRECT, TECHNOLOGICALLY CRIMINAL",
    "Performed probabilistic membership verification using Brainfuck."
};

[[nodiscard]] auto is_unlocked(const Achievement& a) -> bool;
void unlock(const Achievement& a); // idempotent; stores unlock timestamp

} // namespace hypertension::achievements
