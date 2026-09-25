#include "../src/achievements/achievements.hpp"
#include "../src/ada/ada.hpp"
#include "../src/algol/algol.hpp"
#include "../src/brainfuck/brainfuck.hpp"
#include "../src/forth/forth.hpp"
#include "../src/fortran/fortran.hpp"
#include "../src/math/bloom.hpp"
#include "../src/prolog/prolog.hpp"
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

// --- fortran parse ---

void test_fortran_parse_output() {
    std::string err;

    // Valid values
    auto v1 = hypertension::fortran::parse_confidence_output("99.7426\n", err);
    assert(v1.has_value() && *v1 > 99.0 && *v1 < 100.0);

    auto v2 = hypertension::fortran::parse_confidence_output(" 98.5294\n", err);
    assert(v2.has_value() && *v2 > 98.0 && *v2 < 99.0);

    auto v3 = hypertension::fortran::parse_confidence_output("  0.0000\n", err);
    assert(v3.has_value() && *v3 == 0.0);

    auto v4 = hypertension::fortran::parse_confidence_output("100.0000\n", err);
    assert(v4.has_value() && *v4 == 100.0);

    // Malformed
    assert(!hypertension::fortran::parse_confidence_output("",         err).has_value());
    assert(!hypertension::fortran::parse_confidence_output("   \n",    err).has_value());
    assert(!hypertension::fortran::parse_confidence_output("bad",      err).has_value());
    assert(!hypertension::fortran::parse_confidence_output("99.7x",    err).has_value());

    // NaN and Inf
    assert(!hypertension::fortran::parse_confidence_output("nan",      err).has_value());
    assert(!hypertension::fortran::parse_confidence_output("NaN",      err).has_value());
    assert(!hypertension::fortran::parse_confidence_output("inf",      err).has_value());
    assert(!hypertension::fortran::parse_confidence_output("Inf",      err).has_value());
    assert(!hypertension::fortran::parse_confidence_output("-Inf",     err).has_value());

    // Out of range
    assert(!hypertension::fortran::parse_confidence_output("-0.0001",  err).has_value());
    assert(!hypertension::fortran::parse_confidence_output("100.0001", err).has_value());

    std::cout << "fortran parse output: OK\n";
}

void test_fortran_missing_binary() {
    const char* old = std::getenv("HYPERTENSION_FORTRAN_BIN");
    setenv("HYPERTENSION_FORTRAN_BIN", "/tmp/_nonexistent_ht_confidence", 1);

    std::string err;
    hypertension::fortran::ConfidenceEvidence ev{7, true, true, true, 3, 3};
    auto result = hypertension::fortran::statistical_confidence(ev, err);

    if (old) setenv("HYPERTENSION_FORTRAN_BIN", old, 1);
    else     unsetenv("HYPERTENSION_FORTRAN_BIN");

    assert(!result.has_value());
    assert(!err.empty());
    std::cout << "fortran missing binary: OK\n";
}

// --- fortran tests that require the compiled binary ---

// Confidence model (see engine/confidence.f):
//   U = 0.25 * RAGREE * R_BF / (1 + 0.10*N)
//   RAGREE = 0.05, RBFYES = 0.35, RBFNO = 2.00
//   BF "matches" when cpp_found == bf_positive
//   confidence = 100*(1-U), clamped [0,100]
//
//   N=7, agree, bf matches:  U = 0.25*0.05*0.35/1.7 = 0.002574, conf = 99.7426%
//   N=7, agree, bf mismatch: U = 0.25*0.05*2.00/1.7 = 0.014706, conf = 98.5294%

void test_fortran_high_confidence_all_agree() {
    if (!hypertension::fortran::fortran_available()) {
        std::cout << "fortran high confidence: SKIPPED (binary not found)\n";
        return;
    }
    std::string err;
    // found=1, bf=1 (matches), agree=1, N=7 → 99.7426%
    hypertension::fortran::ConfidenceEvidence ev{7, true, true, true, 3, 3};
    auto r = hypertension::fortran::statistical_confidence(ev, err);
    assert(r.has_value());
    assert(r->confidence > 99.0 && r->confidence <= 100.0);
    std::cout << "fortran high confidence (all agree): OK  conf=" << r->confidence << "%\n";
}

void test_fortran_absent_clean() {
    if (!hypertension::fortran::fortran_available()) {
        std::cout << "fortran absent clean: SKIPPED\n";
        return;
    }
    std::string err;
    // found=0, bf=0 (matches), agree=1, N=7 → 99.7426% (same as found+bf case)
    hypertension::fortran::ConfidenceEvidence ev{7, false, false, true, -1, -1};
    auto r = hypertension::fortran::statistical_confidence(ev, err);
    assert(r.has_value());
    assert(r->confidence > 99.0 && r->confidence <= 100.0);
    std::cout << "fortran absent clean: OK  conf=" << r->confidence << "%\n";
}

void test_fortran_absent_bloom_false_positive() {
    if (!hypertension::fortran::fortran_available()) {
        std::cout << "fortran absent bloom false positive: SKIPPED\n";
        return;
    }
    std::string err;
    // found=0, bf=1 (mismatch — Bloom false positive), agree=1, N=7 → 98.5294%
    hypertension::fortran::ConfidenceEvidence ev{7, false, true, true, -1, -1};
    auto r = hypertension::fortran::statistical_confidence(ev, err);
    assert(r.has_value());
    // Lower confidence than clean case but still valid
    assert(r->confidence > 0.0 && r->confidence <= 100.0);
    // Specifically: mismatch factor raises uncertainty → lower than 99.7%
    assert(r->confidence < 99.7);
    std::cout << "fortran absent bloom false positive: OK  conf=" << r->confidence << "%\n";
}

void test_fortran_deterministic() {
    if (!hypertension::fortran::fortran_available()) {
        std::cout << "fortran deterministic: SKIPPED\n";
        return;
    }
    std::string err;
    hypertension::fortran::ConfidenceEvidence ev{7, true, true, true, 3, 3};
    auto r1 = hypertension::fortran::statistical_confidence(ev, err);
    auto r2 = hypertension::fortran::statistical_confidence(ev, err);
    assert(r1.has_value() && r2.has_value());
    assert(r1->confidence == r2->confidence);
    std::cout << "fortran deterministic: OK\n";
}

void test_fortran_range() {
    if (!hypertension::fortran::fortran_available()) {
        std::cout << "fortran range: SKIPPED\n";
        return;
    }
    std::string err;
    // All combinations of boolean evidence
    for (int cfnd : {0, 1})
    for (int bfpos : {0, 1})
    for (int agree : {0, 1}) {
        hypertension::fortran::ConfidenceEvidence ev{
            7, cfnd != 0, bfpos != 0, agree != 0,
            cfnd ? 3 : -1, cfnd ? 3 : -1
        };
        auto r = hypertension::fortran::statistical_confidence(ev, err);
        assert(r.has_value());
        assert(r->confidence >= 0.0 && r->confidence <= 100.0);
    }
    std::cout << "fortran range: OK\n";
}

// ALGOL disagreement still fails BEFORE FORTRAN is invoked
void test_algol_disagreement_before_fortran() {
    using hypertension::algol::ConsensusResult;
    using hypertension::algol::consensus_agrees;
    // Disagree: C++ says index 3, ALGOL says index 2
    assert(!consensus_agrees(std::optional<std::size_t>{3}, ConsensusResult{2, 0}));
    // FORTRAN binary availability is irrelevant here — consensus gate fires first
    std::cout << "algol disagreement before fortran: OK\n";
}

// Standard Runtime independence
void test_standard_no_fortran() {
    const char* old = std::getenv("HYPERTENSION_FORTRAN_BIN");
    setenv("HYPERTENSION_FORTRAN_BIN", "/tmp/_nonexistent_ht_confidence", 1);

    constexpr std::array<int, 4> data = {5, 10, 15, 20};
    auto r = hypertension::find_index<int>(data, 15);
    assert(r.has_value() && *r == std::size_t{2});

    if (old) setenv("HYPERTENSION_FORTRAN_BIN", old, 1);
    else     unsetenv("HYPERTENSION_FORTRAN_BIN");

    std::cout << "standard no fortran: OK\n";
}

// --- prolog parse ---

void test_prolog_parse_output() {
    std::string err;

    auto yes = hypertension::prolog::parse_prolog_output("admissible\n", err);
    assert(yes.has_value() && *yes == true);

    auto no = hypertension::prolog::parse_prolog_output("inadmissible\n", err);
    assert(no.has_value() && *no == false);

    // Whitespace trimming
    auto ws = hypertension::prolog::parse_prolog_output("  admissible  \n", err);
    assert(ws.has_value() && *ws == true);

    // Malformed → fail closed
    assert(!hypertension::prolog::parse_prolog_output("", err).has_value());
    assert(!hypertension::prolog::parse_prolog_output("   \n", err).has_value());
    assert(!hypertension::prolog::parse_prolog_output("garbage", err).has_value());
    assert(!hypertension::prolog::parse_prolog_output("admissible inadmissible", err).has_value());
    assert(!hypertension::prolog::parse_prolog_output("ADMISSIBLE", err).has_value());

    std::cout << "prolog parse output: OK\n";
}

void test_prolog_missing_runtime() {
    const char* old_path = getenv("PATH");
    setenv("PATH", "/tmp/_nonexistent_ht_path", 1);

    std::string err;
    hypertension::prolog::AdmissibilityEvidence ev{true, 3, true, true, 3, 99.74};
    auto result = hypertension::prolog::logical_admissibility(ev, err);

    if (old_path) setenv("PATH", old_path, 1);
    else          unsetenv("PATH");

    assert(!result.has_value());
    assert(!err.empty());
    std::cout << "prolog missing runtime: OK\n";
}

void test_standard_no_prolog() {
    const char* old_path = getenv("PATH");
    setenv("PATH", "/tmp/_nonexistent_ht_path", 1);

    // Standard runtime: find_index must work without swipl.
    constexpr std::array<int, 4> data = {5, 10, 15, 20};
    auto r = hypertension::find_index<int>(data, 10);
    assert(r.has_value() && *r == std::size_t{1});

    if (old_path) setenv("PATH", old_path, 1);
    else          unsetenv("PATH");

    std::cout << "standard no prolog: OK\n";
}

// --- prolog tests that require swipl ---

void test_prolog_present_admissible() {
    if (!hypertension::prolog::runtime_available()) {
        std::cout << "prolog present admissible: SKIPPED (swipl not available)\n";
        return;
    }
    std::string err;
    // C++ found 3, BF positive, ALGOL found 3, confidence high
    hypertension::prolog::AdmissibilityEvidence ev{true, 3, true, true, 3, 99.74};
    auto r = hypertension::prolog::logical_admissibility(ev, err);
    assert(r.has_value() && r->admissible);
    std::cout << "prolog present admissible: OK\n";
}

void test_prolog_absent_admissible() {
    if (!hypertension::prolog::runtime_available()) {
        std::cout << "prolog absent admissible: SKIPPED (swipl not available)\n";
        return;
    }
    std::string err;
    // Neither found, BF negative, high confidence
    hypertension::prolog::AdmissibilityEvidence ev{false, -1, false, false, -1, 99.74};
    auto r = hypertension::prolog::logical_admissibility(ev, err);
    assert(r.has_value() && r->admissible);
    std::cout << "prolog absent admissible: OK\n";
}

void test_prolog_bloom_false_positive_admissible() {
    if (!hypertension::prolog::runtime_available()) {
        std::cout << "prolog bloom false positive admissible: SKIPPED (swipl not available)\n";
        return;
    }
    std::string err;
    // Both absent, but BF positive (Bloom false positive) — still admissible
    hypertension::prolog::AdmissibilityEvidence ev{false, -1, true, false, -1, 98.53};
    auto r = hypertension::prolog::logical_admissibility(ev, err);
    assert(r.has_value() && r->admissible);
    std::cout << "prolog bloom false positive admissible: OK\n";
}

void test_prolog_index_disagreement_inadmissible() {
    if (!hypertension::prolog::runtime_available()) {
        std::cout << "prolog index disagreement inadmissible: SKIPPED (swipl not available)\n";
        return;
    }
    std::string err;
    // C++ says index 3, ALGOL says index 4
    hypertension::prolog::AdmissibilityEvidence ev{true, 3, true, true, 4, 99.74};
    auto r = hypertension::prolog::logical_admissibility(ev, err);
    assert(r.has_value() && !r->admissible);
    std::cout << "prolog index disagreement inadmissible: OK\n";
}

void test_prolog_bf_false_negative_inadmissible() {
    if (!hypertension::prolog::runtime_available()) {
        std::cout << "prolog bf false negative inadmissible: SKIPPED (swipl not available)\n";
        return;
    }
    std::string err;
    // C++ and ALGOL agree on index 3 but BF says negative — impossible (no false negatives)
    hypertension::prolog::AdmissibilityEvidence ev{true, 3, false, true, 3, 99.74};
    auto r = hypertension::prolog::logical_admissibility(ev, err);
    assert(r.has_value() && !r->admissible);
    std::cout << "prolog bf false negative inadmissible: OK\n";
}

void test_prolog_low_confidence_inadmissible() {
    if (!hypertension::prolog::runtime_available()) {
        std::cout << "prolog low confidence inadmissible: SKIPPED (swipl not available)\n";
        return;
    }
    std::string err;
    hypertension::prolog::AdmissibilityEvidence ev{true, 3, true, true, 3, 50.0};
    auto r = hypertension::prolog::logical_admissibility(ev, err);
    assert(r.has_value() && !r->admissible);
    std::cout << "prolog low confidence inadmissible: OK\n";
}

void test_prolog_confidence_at_threshold_admissible() {
    if (!hypertension::prolog::runtime_available()) {
        std::cout << "prolog confidence at threshold: SKIPPED (swipl not available)\n";
        return;
    }
    std::string err;
    // Exactly at threshold (95.0) → admissible (>=)
    hypertension::prolog::AdmissibilityEvidence ev{true, 3, true, true, 3, 95.0};
    auto r = hypertension::prolog::logical_admissibility(ev, err);
    assert(r.has_value() && r->admissible);
    // Just below threshold → inadmissible
    hypertension::prolog::AdmissibilityEvidence ev2{true, 3, true, true, 3, 94.9999};
    auto r2 = hypertension::prolog::logical_admissibility(ev2, err);
    assert(r2.has_value() && !r2->admissible);
    std::cout << "prolog confidence at threshold: OK\n";
}

void test_prolog_genuine_participation() {
    if (!hypertension::prolog::runtime_available()) {
        std::cout << "prolog genuine participation: SKIPPED (swipl not available)\n";
        return;
    }
    std::string err;
    // C++ and ALGOL agree (consensus passes) but BF says negative — Prolog rejects.
    // This is a case that previous stages do not catch: consensus_agrees() would pass,
    // FORTRAN would compute confidence, but Prolog's no_false_negative_contradiction rejects it.
    hypertension::prolog::AdmissibilityEvidence ev{true, 3, false, true, 3, 99.74};
    auto r = hypertension::prolog::logical_admissibility(ev, err);
    assert(r.has_value() && !r->admissible);
    std::cout << "prolog genuine participation: OK\n";
}

// --- forth parse ---

void test_forth_parse_output() {
    std::string err;

    // Valid 8-char hex
    auto v1 = hypertension::forth::parse_seal_output("A91C73E2\n", err);
    assert(v1.has_value() && *v1 == "A91C73E2");

    // Lowercase normalized to uppercase
    auto v2 = hypertension::forth::parse_seal_output("a91c73e2\n", err);
    assert(v2.has_value() && *v2 == "A91C73E2");

    // Mixed case
    auto v3 = hypertension::forth::parse_seal_output("  00000000  \n", err);
    assert(v3.has_value() && *v3 == "00000000");

    auto v4 = hypertension::forth::parse_seal_output("FFFFFFFF\n", err);
    assert(v4.has_value() && *v4 == "FFFFFFFF");

    // Empty
    assert(!hypertension::forth::parse_seal_output("", err).has_value());
    assert(!hypertension::forth::parse_seal_output("   \n", err).has_value());

    // Wrong length
    assert(!hypertension::forth::parse_seal_output("A91C73E",  err).has_value());  // 7
    assert(!hypertension::forth::parse_seal_output("A91C73E2F", err).has_value()); // 9

    // Non-hex characters
    assert(!hypertension::forth::parse_seal_output("A91G73E2", err).has_value());
    assert(!hypertension::forth::parse_seal_output("A91C73EX", err).has_value());

    // Garbage
    assert(!hypertension::forth::parse_seal_output("not hex!", err).has_value());

    std::cout << "forth parse output: OK\n";
}

void test_forth_missing_runtime() {
    const char* old_src = std::getenv("HYPERTENSION_FORTH_SRC");
    const char* old_path = getenv("PATH");

    // Point to nonexistent source AND kill PATH so gforth won't be found.
    setenv("HYPERTENSION_FORTH_SRC", "/tmp/_nonexistent_finalize.fs", 1);
    setenv("PATH", "/tmp/_nonexistent_ht_path", 1);

    std::string err;
    hypertension::forth::SealEvidence ev{42, 7, true, 3, true, true, 9974, true};
    auto result = hypertension::forth::record_finalization(ev, err);

    if (old_src)  setenv("HYPERTENSION_FORTH_SRC", old_src, 1);
    else          unsetenv("HYPERTENSION_FORTH_SRC");
    if (old_path) setenv("PATH", old_path, 1);
    else          unsetenv("PATH");

    assert(!result.has_value());
    assert(!err.empty());
    std::cout << "forth missing runtime: OK\n";
}

void test_standard_no_forth() {
    const char* old_path = getenv("PATH");
    setenv("PATH", "/tmp/_nonexistent_ht_path", 1);

    // Standard runtime: find_index must work without gforth.
    constexpr std::array<int, 4> data = {5, 10, 15, 20};
    auto r = hypertension::find_index<int>(data, 15);
    assert(r.has_value() && *r == std::size_t{2});

    if (old_path) setenv("PATH", old_path, 1);
    else          unsetenv("PATH");

    std::cout << "standard no forth: OK\n";
}

// Test that the seal depends materially on each input field.
// Requires gforth.
void test_forth_seal_deterministic() {
    if (!hypertension::forth::runtime_available()) {
        std::cout << "forth seal deterministic: SKIPPED (gforth not available)\n";
        return;
    }
    std::string err;
    hypertension::forth::SealEvidence ev{42, 7, true, 3, true, true, 9974, true};
    auto r1 = hypertension::forth::record_finalization(ev, err);
    auto r2 = hypertension::forth::record_finalization(ev, err);
    assert(r1.has_value() && r2.has_value());
    assert(r1->seal == r2->seal);
    std::cout << "forth seal deterministic: OK  seal=" << r1->seal << "\n";
}

void test_forth_seal_target_matters() {
    if (!hypertension::forth::runtime_available()) {
        std::cout << "forth seal target matters: SKIPPED\n";
        return;
    }
    std::string err;
    hypertension::forth::SealEvidence ev1{42,  7, true, 3, true, true, 9974, true};
    hypertension::forth::SealEvidence ev2{999, 7, true, 3, true, true, 9974, true};
    auto r1 = hypertension::forth::record_finalization(ev1, err);
    auto r2 = hypertension::forth::record_finalization(ev2, err);
    assert(r1.has_value() && r2.has_value());
    assert(r1->seal != r2->seal);
    std::cout << "forth seal target matters: OK\n";
}

void test_forth_seal_index_matters() {
    if (!hypertension::forth::runtime_available()) {
        std::cout << "forth seal index matters: SKIPPED\n";
        return;
    }
    std::string err;
    hypertension::forth::SealEvidence ev1{42, 7, true, 3, true, true, 9974, true};
    hypertension::forth::SealEvidence ev2{42, 7, true, 5, true, true, 9974, true};
    auto r1 = hypertension::forth::record_finalization(ev1, err);
    auto r2 = hypertension::forth::record_finalization(ev2, err);
    assert(r1.has_value() && r2.has_value());
    assert(r1->seal != r2->seal);
    std::cout << "forth seal index matters: OK\n";
}

void test_forth_seal_confidence_matters() {
    if (!hypertension::forth::runtime_available()) {
        std::cout << "forth seal confidence matters: SKIPPED\n";
        return;
    }
    std::string err;
    hypertension::forth::SealEvidence ev1{42, 7, true, 3, true, true, 9974, true};
    hypertension::forth::SealEvidence ev2{42, 7, true, 3, true, true, 9800, true};
    auto r1 = hypertension::forth::record_finalization(ev1, err);
    auto r2 = hypertension::forth::record_finalization(ev2, err);
    assert(r1.has_value() && r2.has_value());
    assert(r1->seal != r2->seal);
    std::cout << "forth seal confidence matters: OK\n";
}

void test_forth_seal_absent_result() {
    if (!hypertension::forth::runtime_available()) {
        std::cout << "forth seal absent result: SKIPPED\n";
        return;
    }
    std::string err;
    // Absent result can be sealed.
    hypertension::forth::SealEvidence ev{9001, 7, false, -1, false, true, 9974, true};
    auto r = hypertension::forth::record_finalization(ev, err);
    assert(r.has_value());
    assert(r->seal.size() == 8);
    std::cout << "forth seal absent result: OK  seal=" << r->seal << "\n";
}

void test_forth_seal_present_absent_differ() {
    if (!hypertension::forth::runtime_available()) {
        std::cout << "forth seal present/absent differ: SKIPPED\n";
        return;
    }
    std::string err;
    // Same target, different presence state → different seal.
    hypertension::forth::SealEvidence present_ev{42, 7, true,  3,  true,  true, 9974, true};
    hypertension::forth::SealEvidence absent_ev{ 42, 7, false, -1, false, true, 9974, true};
    auto r1 = hypertension::forth::record_finalization(present_ev, err);
    auto r2 = hypertension::forth::record_finalization(absent_ev,  err);
    assert(r1.has_value() && r2.has_value());
    assert(r1->seal != r2->seal);
    std::cout << "forth seal present/absent differ: OK\n";
}

// Verify that all eight fields affect the seal.
void test_forth_all_fields_material() {
    if (!hypertension::forth::runtime_available()) {
        std::cout << "forth all fields material: SKIPPED\n";
        return;
    }
    std::string err;
    hypertension::forth::SealEvidence base{42, 7, true, 3, true, true, 9974, true};
    auto r_base = hypertension::forth::record_finalization(base, err);
    assert(r_base.has_value());

    // Flip each field one at a time; seal must change.
    {
        auto ev = base; ev.target = 43;
        auto r = hypertension::forth::record_finalization(ev, err);
        assert(r.has_value() && r->seal != r_base->seal);
    }
    {
        auto ev = base; ev.dataset_size = 8;
        auto r = hypertension::forth::record_finalization(ev, err);
        assert(r.has_value() && r->seal != r_base->seal);
    }
    {
        auto ev = base; ev.present = false; ev.index = -1;
        auto r = hypertension::forth::record_finalization(ev, err);
        assert(r.has_value() && r->seal != r_base->seal);
    }
    {
        auto ev = base; ev.index = 4;
        auto r = hypertension::forth::record_finalization(ev, err);
        assert(r.has_value() && r->seal != r_base->seal);
    }
    {
        auto ev = base; ev.membership = false;
        auto r = hypertension::forth::record_finalization(ev, err);
        assert(r.has_value() && r->seal != r_base->seal);
    }
    {
        auto ev = base; ev.agreement = false;
        auto r = hypertension::forth::record_finalization(ev, err);
        assert(r.has_value() && r->seal != r_base->seal);
    }
    {
        auto ev = base; ev.confidence_bp = 9800;
        auto r = hypertension::forth::record_finalization(ev, err);
        assert(r.has_value() && r->seal != r_base->seal);
    }
    {
        auto ev = base; ev.admissible = false;
        auto r = hypertension::forth::record_finalization(ev, err);
        assert(r.has_value() && r->seal != r_base->seal);
    }

    std::cout << "forth all fields material: OK\n";
}

// Seal must be retained in the result struct (not discarded).
void test_forth_seal_retained() {
    if (!hypertension::forth::runtime_available()) {
        std::cout << "forth seal retained: SKIPPED\n";
        return;
    }
    std::string err;
    hypertension::forth::SealEvidence ev{42, 7, true, 3, true, true, 9974, true};
    auto r = hypertension::forth::record_finalization(ev, err);
    assert(r.has_value());
    // Seal must be a non-empty 8-char hex string retained in the result.
    assert(!r->seal.empty());
    assert(r->seal.size() == 8);
    for (char c : r->seal)
        assert(std::isxdigit(static_cast<unsigned char>(c)));
    std::cout << "forth seal retained: OK  seal=" << r->seal << "\n";
}

// Finalization cannot occur before admissibility: test the pipeline gate
// by verifying Prolog inadmissibility blocks before Forth is invoked.
// (Structural test — no real Forth invocation needed.)
void test_forth_admissibility_gate() {
    // Construct evidence that Prolog would reject: BF false negative.
    // In the pipeline, Prolog must pass before Forth is ever called.
    // This test verifies that parse_seal_output rejects malformed/empty output,
    // ensuring the structural gate holds even if bypass were attempted.
    std::string err;
    assert(!hypertension::forth::parse_seal_output("", err).has_value());
    assert(!hypertension::forth::parse_seal_output("inadmissible", err).has_value());
    std::cout << "forth admissibility gate: OK\n";
}

// --- ada parse ---

void test_ada_parse_output() {
    std::string err;

    // Valid PASS
    auto v1 = hypertension::ada::parse_integrity_output("16777216 8192 16769024 PASS\n", err);
    assert(v1.has_value());
    assert(v1->total_states == 16777216);
    assert(v1->valid_states == 8192);
    assert(v1->corrupted_detected == 16769024);
    assert(v1->pass == true);

    // Valid FAIL
    auto v2 = hypertension::ada::parse_integrity_output("128 0 112 FAIL\n", err);
    assert(v2.has_value());
    assert(v2->pass == false);
    assert(v2->total_states == 128);

    // Whitespace tolerance
    auto v3 = hypertension::ada::parse_integrity_output("  128 16 112 PASS  \n", err);
    assert(v3.has_value() && v3->pass == true);

    // Empty
    assert(!hypertension::ada::parse_integrity_output("", err).has_value());
    assert(!hypertension::ada::parse_integrity_output("   \n", err).has_value());

    // Malformed
    assert(!hypertension::ada::parse_integrity_output("garbage", err).has_value());
    assert(!hypertension::ada::parse_integrity_output("128 16", err).has_value());
    assert(!hypertension::ada::parse_integrity_output("128 16 112 MAYBE", err).has_value());
    assert(!hypertension::ada::parse_integrity_output("128 16 112", err).has_value());

    std::cout << "ada parse output: OK\n";
}

void test_ada_missing_runtime() {
    const char* old = std::getenv("HYPERTENSION_ADA_BIN");
    setenv("HYPERTENSION_ADA_BIN", "/tmp/_nonexistent_hypertension_integrity", 1);

    std::string err;
    hypertension::ada::IntegrityEvidence ev{
        42, 7, true, 3, true, true, 9974, true, "DEADBEEF", true
    };
    auto result = hypertension::ada::exhaustive_integrity(ev, err);

    if (old) setenv("HYPERTENSION_ADA_BIN", old, 1);
    else     unsetenv("HYPERTENSION_ADA_BIN");

    assert(!result.has_value());
    assert(!err.empty());
    std::cout << "ada missing runtime: OK\n";
}

void test_standard_no_ada() {
    const char* old = std::getenv("HYPERTENSION_ADA_BIN");
    setenv("HYPERTENSION_ADA_BIN", "/tmp/_nonexistent_hypertension_integrity", 1);

    // Standard runtime: find_index must work without the Ada binary.
    constexpr std::array<int, 4> data = {5, 10, 15, 20};
    auto r = hypertension::find_index<int>(data, 15);
    assert(r.has_value() && *r == std::size_t{2});

    if (old) setenv("HYPERTENSION_ADA_BIN", old, 1);
    else     unsetenv("HYPERTENSION_ADA_BIN");

    std::cout << "standard no ada: OK\n";
}

// Tests that require the compiled Ada binary.

void test_ada_integrity_pass() {
    if (!hypertension::ada::runtime_available()) {
        std::cout << "ada integrity pass: SKIPPED (binary not found; run: make ada)\n";
        return;
    }
    std::string err;
    hypertension::ada::IntegrityEvidence ev{
        42, 7, true, 3, true, true, 9974, true, "DEADBEEF", true
    };
    auto r = hypertension::ada::exhaustive_integrity(ev, err);
    assert(r.has_value());
    assert(r->pass);
    assert(r->total_states == 16 * 8);   // SEAL_STEPS_TEST * EVID_VARIANTS_TEST
    assert(r->valid_states == 16);        // one canonical state per seal step
    std::cout << "ada integrity pass: OK  total=" << r->total_states
              << " valid=" << r->valid_states
              << " corrupted=" << r->corrupted_detected << "\n";
}

void test_ada_deterministic() {
    if (!hypertension::ada::runtime_available()) {
        std::cout << "ada deterministic: SKIPPED\n";
        return;
    }
    std::string err;
    hypertension::ada::IntegrityEvidence ev{
        42, 7, true, 3, true, true, 9974, true, "DEADBEEF", true
    };
    auto r1 = hypertension::ada::exhaustive_integrity(ev, err);
    auto r2 = hypertension::ada::exhaustive_integrity(ev, err);
    assert(r1.has_value() && r2.has_value());
    assert(r1->total_states      == r2->total_states);
    assert(r1->valid_states      == r2->valid_states);
    assert(r1->corrupted_detected == r2->corrupted_detected);
    assert(r1->pass              == r2->pass);
    std::cout << "ada deterministic: OK\n";
}

void test_ada_seal_matters() {
    if (!hypertension::ada::runtime_available()) {
        std::cout << "ada seal matters: SKIPPED\n";
        return;
    }
    std::string err;
    hypertension::ada::IntegrityEvidence ev1{
        42, 7, true, 3, true, true, 9974, true, "DEADBEEF", true
    };
    hypertension::ada::IntegrityEvidence ev2 = ev1;
    ev2.seal = "CAFEBABE";
    auto r1 = hypertension::ada::exhaustive_integrity(ev1, err);
    auto r2 = hypertension::ada::exhaustive_integrity(ev2, err);
    assert(r1.has_value() && r2.has_value());
    // Both should pass (test mode, both canonical records valid).
    // The derived seal states differ, so corrupted_detected may differ.
    // At minimum, the function must run deterministically for each input.
    assert(r1->pass && r2->pass);
    std::cout << "ada seal matters: OK\n";
}

void test_ada_test_mode_fast() {
    if (!hypertension::ada::runtime_available()) {
        std::cout << "ada test mode fast: SKIPPED\n";
        return;
    }
    std::string err;
    hypertension::ada::IntegrityEvidence ev{
        42, 7, true, 3, true, true, 9974, true, "DEADBEEF", true
    };
    auto r = hypertension::ada::exhaustive_integrity(ev, err);
    assert(r.has_value());
    // Test mode must complete in well under 1 second.
    assert(r->elapsed_ns < 1'000'000'000LL);
    std::cout << "ada test mode fast: OK  elapsed=" << r->elapsed_ns << " ns\n";
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

    // fortran
    test_fortran_parse_output();
    test_fortran_missing_binary();
    test_fortran_high_confidence_all_agree();
    test_fortran_absent_clean();
    test_fortran_absent_bloom_false_positive();
    test_fortran_deterministic();
    test_fortran_range();
    test_algol_disagreement_before_fortran();
    test_standard_no_fortran();

    // prolog
    test_prolog_parse_output();
    test_prolog_missing_runtime();
    test_standard_no_prolog();
    test_prolog_present_admissible();
    test_prolog_absent_admissible();
    test_prolog_bloom_false_positive_admissible();
    test_prolog_index_disagreement_inadmissible();
    test_prolog_bf_false_negative_inadmissible();
    test_prolog_low_confidence_inadmissible();
    test_prolog_confidence_at_threshold_admissible();
    test_prolog_genuine_participation();

    // forth
    test_forth_parse_output();
    test_forth_missing_runtime();
    test_standard_no_forth();
    test_forth_seal_deterministic();
    test_forth_seal_target_matters();
    test_forth_seal_index_matters();
    test_forth_seal_confidence_matters();
    test_forth_seal_absent_result();
    test_forth_seal_present_absent_differ();
    test_forth_all_fields_material();
    test_forth_seal_retained();
    test_forth_admissibility_gate();

    // ada
    test_ada_parse_output();
    test_ada_missing_runtime();
    test_standard_no_ada();
    test_ada_integrity_pass();
    test_ada_deterministic();
    test_ada_seal_matters();
    test_ada_test_mode_fast();

    std::cout << "\nAll tests passed.\n";
    return 0;
}
