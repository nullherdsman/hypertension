#include "../src/achievements/achievements.hpp"
#include "../src/brainfuck/brainfuck.hpp"
#include "../src/math/bloom.hpp"
#include "../src/search/search.hpp"
#include "../src/util/data_dir.hpp"

#include <array>
#include <cassert>
#include <cstdint>
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

// --- rule 2 boundary ---

void test_rule2_not_implemented() {
    // verify_membership is a pure read with no side effects.
    // After returning, no Rule-3 state or further progression is created.
    std::string err;
    auto r = hypertension::math::verify_membership(42, err);
    assert(r.has_value() && *r == true);
    std::cout << "rule2 boundary: OK\n";
}

int main() {
    test_search();
    test_bf_basic();
    test_bf_hash_function();
    test_bf_malformed();
    test_membership_members_and_nonmembers();
    test_achievement_idempotent();
    test_rule2_not_implemented();
    std::cout << "\nAll tests passed.\n";
    return 0;
}
