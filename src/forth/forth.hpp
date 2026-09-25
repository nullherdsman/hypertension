#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace hypertension::forth {

// Canonical verification record fields passed to Forth.
//
// Confidence is stored in basis points to keep the canonical record
// independent of locale-sensitive floating-point text.
// index is -1 when the target is absent.
struct SealEvidence {
    int  target;
    int  dataset_size;
    bool present;
    int  index;          // 0-based; -1 if absent
    bool membership;     // Brainfuck filter result
    bool agreement;      // ALGOL independent search agreed
    int  confidence_bp;  // e.g. 9974 for 99.74%
    bool admissible;     // Prolog declared admissible
};

struct SealResult {
    std::string seal;     // 8 uppercase hex characters
    long long elapsed_ns;
};

// Returns false if gforth is not in PATH.
[[nodiscard]] bool runtime_available();

// Parses one line of gforth stdout.  Exposed for unit tests.
// Accepts exactly 8 hex characters (upper or lower case).
// Returns nullopt on any other input.
[[nodiscard]] auto parse_seal_output(std::string_view raw, std::string& error)
    -> std::optional<std::string>;

// Run the Forth record finalization authority.
// Returns nullopt (with error set) if gforth is unavailable or output is invalid.
// Admissibility must be true; calling with admissible=false is a logic error.
[[nodiscard]] auto record_finalization(
    const SealEvidence& evidence,
    std::string& error
) -> std::optional<SealResult>;

} // namespace hypertension::forth
