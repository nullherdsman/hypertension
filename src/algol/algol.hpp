#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace hypertension::algol {

struct ConsensusResult {
    std::optional<int> index;   // nullopt = target absent
    long long elapsed_ns;
};

// Returns false if a68g is not in PATH.
[[nodiscard]] bool runtime_available();

// Parses a single line of a68g stdout output.  Exposed for unit tests.
[[nodiscard]] auto parse_algol_output(std::string_view raw, std::string& error)
    -> std::optional<int>;

// Returns true iff both searches agree on presence and position.
[[nodiscard]] bool consensus_agrees(
    std::optional<std::size_t> cpp_result,
    const ConsensusResult& algol_result
);

// Run the Theta(n^3) independent consensus search via a68g.
// Returns nullopt (with error set) if a68g is unavailable or execution fails.
[[nodiscard]] auto consensus_search(
    std::span<const int> dataset,
    int target,
    std::string& error
) -> std::optional<ConsensusResult>;

} // namespace hypertension::algol
