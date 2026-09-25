#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace hypertension::fortran {

struct ConfidenceEvidence {
    int  n;           // dataset size
    bool cpp_found;   // C++ found target
    bool bf_positive; // Brainfuck membership positive
    bool agree;       // C++ and ALGOL agreed
    int  cpp_index;   // -1 if absent
    int  algol_index; // -1 if absent
};

struct ConfidenceResult {
    double    confidence;  // 0–100
    long long elapsed_ns;
};

// Returns false if build/hypertension-confidence is not found.
[[nodiscard]] bool fortran_available();

// Parses one line of confidence-binary stdout.  Exposed for unit tests.
// Rejects NaN, infinity, out-of-range, empty, and malformed text.
[[nodiscard]] auto parse_confidence_output(std::string_view raw, std::string& error)
    -> std::optional<double>;

// Run the FORTRAN 77 statistical confidence authority.
// Returns nullopt (with error set) if binary unavailable or output invalid.
[[nodiscard]] auto statistical_confidence(
    const ConfidenceEvidence& evidence,
    std::string& error
) -> std::optional<ConfidenceResult>;

} // namespace hypertension::fortran
