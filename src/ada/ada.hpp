#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace hypertension::ada {

struct IntegrityEvidence {
    int  target;
    int  dataset_size;
    bool present;
    int  index;          // 0-based; -1 if absent
    bool membership;
    bool agreement;
    int  confidence_bp;  // e.g. 9974 for 99.74%
    bool admissible;
    std::string seal;    // 8 uppercase hex characters from Forth stage
    bool test_mode = false;
};

struct IntegrityResult {
    long long total_states;
    long long valid_states;
    long long corrupted_detected;
    bool      pass;
    long long elapsed_ns;
};

bool runtime_available();
auto parse_integrity_output(std::string_view raw, std::string& error) -> std::optional<IntegrityResult>;
auto exhaustive_integrity(const IntegrityEvidence& evidence, std::string& error) -> std::optional<IntegrityResult>;

} // namespace hypertension::ada
