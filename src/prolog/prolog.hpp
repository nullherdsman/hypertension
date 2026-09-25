#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace hypertension::prolog {

struct AdmissibilityEvidence {
    bool   cpp_found;
    int    cpp_index;    // -1 if absent
    bool   bf_positive;
    bool   algol_found;
    int    algol_index;  // -1 if absent
    double confidence;
};

struct AdmissibilityResult {
    bool      admissible;
    long long elapsed_ns;
};

// Returns false if swipl is not in PATH.
[[nodiscard]] bool runtime_available();

// Parses one line of swipl stdout. Exposed for unit tests.
// Returns true for "admissible", false for "inadmissible", nullopt otherwise.
[[nodiscard]] auto parse_prolog_output(std::string_view raw, std::string& error)
    -> std::optional<bool>;

// Run the Prolog logical admissibility authority.
// Returns nullopt (with error set) if swipl is unavailable or output is invalid.
[[nodiscard]] auto logical_admissibility(
    const AdmissibilityEvidence& evidence,
    std::string& error
) -> std::optional<AdmissibilityResult>;

} // namespace hypertension::prolog
