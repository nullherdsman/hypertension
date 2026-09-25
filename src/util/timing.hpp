#pragma once

#include <chrono>
#include <string>

namespace hypertension {

class Timer {
    std::chrono::high_resolution_clock::time_point start_;

public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    [[nodiscard]] auto elapsed_ns() const -> long long {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_).count();
    }
};

[[nodiscard]] inline auto format_time(long long ns) -> std::string {
    if (ns < 1000LL)       return std::to_string(ns) + " ns";
    if (ns < 1000000LL)    return std::to_string(ns / 1000) + " \xc2\xb5s";   // µs (UTF-8)
    if (ns < 1000000000LL) return std::to_string(ns / 1000000) + " ms";
    return std::to_string(ns / 1000000000) + " s";
}

} // namespace hypertension
