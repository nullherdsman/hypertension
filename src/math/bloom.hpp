#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace hypertension::math {

// Canonical source: engine/membership.bf
constexpr std::string_view MEMBERSHIP_BF = ",[->+++<]>+++++++.";

// Pure: run bf_program and test membership of value in the dataset.
[[nodiscard]] auto bloom_check(std::string_view bf_program, int value, std::string& error)
    -> std::optional<bool>;

// Uses embedded MEMBERSHIP_BF.
[[nodiscard]] auto verify_membership(int value, std::string& error) -> std::optional<bool>;

} // namespace hypertension::math
