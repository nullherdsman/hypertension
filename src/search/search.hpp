#pragma once

#include <concepts>
#include <cstddef>
#include <optional>
#include <span>

namespace hypertension {

template <std::equality_comparable T>
[[nodiscard]]
constexpr auto find_index(
    std::span<const T> haystack,
    const T& needle
) -> std::optional<std::size_t>
{
    for (std::size_t i = 0; i < haystack.size(); ++i) {
        if (haystack[i] == needle)
            return i;
    }
    return std::nullopt;
}

} // namespace hypertension
