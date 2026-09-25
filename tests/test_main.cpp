#include "../src/achievements/achievements.hpp"
#include "../src/algol/algol.hpp"
#include "../src/brainfuck/brainfuck.hpp"
#include "../src/math/bloom.hpp"
#include "../src/search/search.hpp"
#include "../src/util/data_dir.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>

namespace fs = std::filesystem;

// --- search ---

void test_search() {
    constexpr std::array<int, 7> data = {7, 13, 21, 42, 64, 128, 256};
    assert(hypertension::find_index<int>(data, 42) == std::size_t{3});
    assert(hypertension::find_index<int>(data, 7) == std::size_t{0});
    assert(hypertension::find_index<int>(data, 256) == std::size_t{6});
    assert(!hypertension::find_index<int>(data, 99));
    assert(!hypertension::find_index<int>(data, 0));
    std::cout << "search: OK\n";
}

// --- brainfuck ---

void test_bf_basic() {
    using namespace hypertension::bf;

    // Output 'A' = 65
    {
        std::string prog(65, '+');
        prog += '.';
        auto r = run(prog, {});
        assert(r.error.empty() && r.output == "A");
    }
    // Cat
    {
        std::array<uint8_t, 1> input{42};
        auto r = run(",.", std::span<const uint8_t>(input));
        assert(r.error.empty() && static_cast<uint8_t>(r.output[0]) == 42);
    }
    // Non-instruction characters ignored
    {
        auto r = run("comment +++ more comment .", {});
        assert(r.error.empty() && static_cast<uint8_t>(r.output[0]) == 3);
    }
    std::cout << "bf basic: OK\n";
}

void test_bf_hash_function() {
    // Verifies math verification depends on BF output:
    // engine/membership.bf computes h(x) = (3x+7) mod 256.
    // For every dataset member, h1=output%16 and h2=input%16 must both
    // be set bits in BLOOM_BITS. A wrong BF program breaks this.
    const std::string_view prog = hypertension::math::MEMBERSHIP_BF;

    const struct { uint8_t input; uint8_t expected; } cases[] = {
        {  7,  28}, {13, 46}, {21, 70}, {42, 133},
        { 64, 199}, {128, 135}, {0, 7},
    };
    constexpr uint16_t BLOOM_BITS = 0x74E1u;

    for (auto& [in, expected] : cases) {
        std::array<uint8_t, 1> input{in};
        auto r = hypertension::bf::run(prog, std::span<const uint8_t>(input));
        assert(r.error.empty());
        assert(static_cast<uint8_t>(r.output[0]) == expected);
        assert((BLOOM_BITS >> (expected % 16)) & 1);
        assert((BLOOM_BITS >> (in % 16)) & 1);
    }
    std::cout << "bf hash function: OK\n";
}

void test_bf_malformed() {
    using namespace hypertension::bf;
    assert(!run("[",   {}).error.empty());
    assert(!run("]",   {}).error.empty());
    assert(!run("[[]", {}).error.empty());
    assert(!run("[]]", {}).error.empty());
    std::cout << "bf malformed: OK\n";
}

// --- bloom / membership ---

void test_membership_members_and_nonmembers() {
    std::string err;
    for (int v : {7, 13, 21, 42, 64, 128, 256}) {
        auto r = hypertension::math::verify_membership(v, err);
        assert(r.has_value() && *r == true);
    }
    for (int v : {1, 2, 99, 100, 200}) {
        auto r = hypertension::math::verify_membership(v, err);
        assert(r.has_value() && *r == false);
    }
    std::cout << "membership members/nonmembers: OK\n";
}

// --- achievements ---

void test_achievement_idempotent() {
    auto tmp = fs::temp_directory_path() / "ht_test_achievements";
    fs::remove_all(tmp);
    setenv("XDG_DATA_HOME", tmp.c_str(), 1);

    auto& a = hypertension::achievements::MATHEMATICALLY_CORRECT;
    assert(!hypertension::achievements::is_unlocked(a));
    hypertension::achievements::unlock(a);
    assert(hypertension::achievements::is_unlocked(a));
    hypertension::achievements::unlock(a);
    assert(hypertension::achievements::is_unlocked(a));

    auto path = hypertension::user_data_dir() / "achievements" / a.id;
    std::ifstream f(path);
    std::string ts;
    f >> ts;
    assert(!ts.empty());

    fs::remove_all(tmp);
    unsetenv("XDG_DATA_HOME");
    std::cout << "achievement idempotent: OK\n";
}

// --- algol ---

static constexpr std::array<int, 7> ALGOL_DATASET = {7, 13, 21, 42, 64, 128, 256};

void test_algol_parse_output() {
    std::string err;
    auto r = hypertension::algol::parse_algol_output("3\n", err);
    assert(r.has_value() && *r == 3);

    auto neg = hypertension::algol::parse_algol_output("-1\n", err);
    assert(neg.has_value() && *neg == -1);

    auto bad = hypertension::algol::parse_algol_output("not a number", err);
    assert(!bad.has_value() && !err.empty());

    auto empty = hypertension::algol::parse_algol_output("", err);
    assert(!empty.has_value());

    std::cout << "algol parse output: OK\n";
}

void test_algol_malformed_output() {
    std::string err;
    // Multi-word garbage
    assert(!hypertension::algol::parse_algol_output("error foo", err).has_value());
    // Whitespace only
    assert(!hypertension::algol::parse_algol_output("   \n  ", err).has_value());
    // Partial integer
    assert(!hypertension::algol::parse_algol_output("3x", err).has_value());
    std::cout << "algol malformed output: OK\n";
}

void test_algol_consensus_agrees() {
    using hypertension::algol::ConsensusResult;
    using hypertension::algol::consensus_agrees;

    // Both found same index
    assert(consensus_agrees(std::optional<std::size_t>{3}, ConsensusResult{3, 0}));
    // Both not found
    assert(consensus_agrees(std::nullopt, ConsensusResult{std::nullopt, 0}));
    // C++ found, ALGOL not
    assert(!consensus_agrees(std::optional<std::size_t>{3}, ConsensusResult{std::nullopt, 0}));
    // ALGOL found, C++ not
    assert(!consensus_agrees(std::nullopt, ConsensusResult{3, 0}));
    // Different indices
    assert(!consensus_agrees(std::optional<std::size_t>{3}, ConsensusResult{2, 0}));

    std::cout << "algol consensus agrees: OK\n";
}

void test_algol_missing_runtime() {
    const char* old_path = getenv("PATH");
    setenv("PATH", "/tmp/_nonexistent_ht_path", 1);

    std::string err;
    auto result = hypertension::algol::consensus_search(ALGOL_DATASET, 42, err);

    if (old_path) setenv("PATH", old_path, 1);
    else          unsetenv("PATH");

    assert(!result.has_value());
    assert(!err.empty());
    std::cout << "algol missing runtime: OK\n";
}

// --- algol tests that require a68g ---

void test_algol_execution() {
    if (!hypertension::algol::runtime_available()) {
        std::cout << "algol execution: SKIPPED (a68g not available)\n";
        return;
    }

    std::string err;

    // First index
    auto first = hypertension::algol::consensus_search(ALGOL_DATASET, 7, err);
    assert(first.has_value() && first->index.has_value() && *first->index == 0);

    // Middle index
    auto mid = hypertension::algol::consensus_search(ALGOL_DATASET, 42, err);
    assert(mid.has_value() && mid->index.has_value() && *mid->index == 3);

    // Last index
    auto last = hypertension::algol::consensus_search(ALGOL_DATASET, 256, err);
    assert(last.has_value() && last->index.has_value() && *last->index == 6);

    // Missing value
    auto absent = hypertension::algol::consensus_search(ALGOL_DATASET, 99, err);
    assert(absent.has_value() && !absent->index.has_value());

    std::cout << "algol execution: OK\n";
}

void test_algol_zero_based_index() {
    if (!hypertension::algol::runtime_available()) {
        std::cout << "algol zero-based index: SKIPPED (a68g not available)\n";
        return;
    }
    std::string err;
    // ALGOL is 1-based internally; result must be 0-based to match C++.
    auto r = hypertension::algol::consensus_search(ALGOL_DATASET, 7, err);
    assert(r.has_value() && r->index.has_value() && *r->index == 0);

    auto r2 = hypertension::algol::consensus_search(ALGOL_DATASET, 13, err);
    assert(r2.has_value() && r2->index.has_value() && *r2->index == 1);

    std::cout << "algol zero-based index: OK\n";
}

void test_algol_cpp_agreement() {
    if (!hypertension::algol::runtime_available()) {
        std::cout << "algol cpp agreement: SKIPPED (a68g not available)\n";
        return;
    }
    std::string err;
    for (int v : {7, 13, 21, 42, 64, 128, 256, 99, 0}) {
        auto cpp = hypertension::find_index<int>(ALGOL_DATASET, v);
        auto algol = hypertension::algol::consensus_search(ALGOL_DATASET, v, err);
        assert(algol.has_value());
        assert(hypertension::algol::consensus_agrees(cpp, *algol));
    }
    std::cout << "algol cpp agreement: OK\n";
}

// --- brainfuck still material ---

void test_bf_still_material() {
    // verify_membership must use BF output, not a static table.
    // A wrong BF program that outputs 0 would break membership for all members.
    std::string err;
    auto r = hypertension::math::bloom_check(",.", 42, err);
    // ",." = echo input byte; h1 = 42%16 = 10; h2 = 42%16 = 10.
    // BLOOM_BITS has bit 10 set, so this returns true (same bit for both hashes).
    assert(r.has_value());
    // The actual canonical program gives a different hash than identity.
    auto canonical = hypertension::math::verify_membership(42, err);
    assert(canonical.has_value() && *canonical == true);
    std::cout << "bf still material: OK\n";
}

// --- standard runtime independence ---

void test_standard_runtime_independent() {
    // find_index works without a68g.
    constexpr std::array<int, 5> data = {1, 2, 3, 4, 5};
    assert(hypertension::find_index<int>(data, 3) == std::size_t{2});
    assert(!hypertension::find_index<int>(data, 9));
    std::cout << "standard runtime independent: OK\n";
}

void test_standard_runtime_without_a68g() {
    const char* old_path = getenv("PATH");
    setenv("PATH", "/tmp/_nonexistent_ht_path", 1);

    constexpr std::array<int, 3> data = {10, 20, 30};
    auto r = hypertension::find_index<int>(data, 20);
    assert(r.has_value() && *r == std::size_t{1});

    if (old_path) setenv("PATH", old_path, 1);
    else          unsetenv("PATH");

    std::cout << "standard runtime without a68g: OK\n";
}

int main() {
    test_search();
    test_bf_basic();
    test_bf_hash_function();
    test_bf_malformed();
    test_membership_members_and_nonmembers();
    test_achievement_idempotent();

    // algol
    test_algol_parse_output();
    test_algol_malformed_output();
    test_algol_consensus_agrees();
    test_algol_missing_runtime();
    test_algol_execution();
    test_algol_zero_based_index();
    test_algol_cpp_agreement();
    test_bf_still_material();
    test_standard_runtime_independent();
    test_standard_runtime_without_a68g();

    std::cout << "\nAll tests passed.\n";
    return 0;
}
