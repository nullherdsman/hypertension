#pragma once

#include <chrono>

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

} // namespace hypertension
