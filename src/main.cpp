#include "achievements/achievements.hpp"
#include "algol/algol.hpp"
#include "cli/cli.hpp"
#include "math/bloom.hpp"
#include "runtime/mode.hpp"
#include "search/search.hpp"
#include "util/timing.hpp"

#include <array>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>

namespace {

constexpr std::string_view VERSION = "0.1.0";
constexpr std::array<int, 7> DATASET = {7, 13, 21, 42, 64, 128, 256};

void print_help() {
    std::cout << "HyperTension " << VERSION << "\n\n"
              << "Usage:\n"
              << "  hypertension search <integer>\n"
              << "  hypertension search <integer> --verified\n"
              << "  hypertension --help\n"
              << "  hypertension --version\n";
}

void print_version() {
    std::cout << "HyperTension " << VERSION << '\n';
}

int run_standard_search(int value, bool verified) {
    std::cout << "HyperTension\n\n";

    hypertension::Timer timer;
    auto result = hypertension::find_index<int>(DATASET, value);
    auto ns = timer.elapsed_ns();

    if (result)
        std::cout << "Found " << value << " at index " << *result << ".\n";
    else
        std::cout << value << " was not found.\n";

    std::cout << "Latency: " << ns << " ns\n";

    if (verified) {
        std::cout << '\n'
                  << "Enhanced verification is not enabled.\n\n"
                  << "Enable HyperTension Extended Runtime? [y/N] ";

        std::string answer;
        std::getline(std::cin, answer);

        if (answer == "y" || answer == "Y") {
            hypertension::save_runtime_mode(hypertension::RuntimeMode::Extended);
            std::cout << "\nEnabling Extended Runtime...\n\n"
                      << "✓ Search state initialized\n"
                      << "✓ Verification state initialized\n"
                      << "✓ Installation registered\n\n"
                      << "Extended Runtime enabled.\n\n"
                      << "This installation can no longer use Standard Runtime.\n"
                      << "Run the command again to continue.\n";
        }
    }

    return result ? 0 : 1;
}

int run_extended_search(int value) {
    std::cout << "HyperTension Extended Runtime\n"
              << "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
                 "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\n\n";

    // Stage 1: C++ O(n) primary search
    hypertension::Timer cpp_timer;
    auto cpp_result = hypertension::find_index<int>(DATASET, value);
    auto cpp_ns = cpp_timer.elapsed_ns();

    // Stage 2: Brainfuck probabilistic membership
    std::string bf_error;
    hypertension::Timer bf_timer;
    auto bloom_result = hypertension::math::verify_membership(value, bf_error);
    auto bf_ns = bf_timer.elapsed_ns();

    if (!bloom_result.has_value()) {
        std::cerr << "error: Brainfuck membership filter failed: " << bf_error << '\n';
        return 2;
    }

    // Stage 3: ALGOL 68 Theta(n^3) independent consensus search
    std::string algol_error;
    auto algol_result = hypertension::algol::consensus_search(DATASET, value, algol_error);

    // Print timing table
    std::cout << "\xe2\x9c\x93 Primary search                C++23       "
              << hypertension::format_time(cpp_ns) << "\n";
    std::cout << "\xe2\x9c\x93 Membership verification      Brainfuck    "
              << hypertension::format_time(bf_ns) << "\n";

    if (!algol_result.has_value()) {
        std::cout << "\nALGOL 68 consensus engine unavailable.\n\n"
                  << "Required runtime: a68g\n";
        return 2;
    }

    std::cout << "\xe2\x9c\x93 Independent consensus        ALGOL 68      "
              << hypertension::format_time(algol_result->elapsed_ns) << "\n\n";

    // Consensus check: C++ and ALGOL must agree
    if (!hypertension::algol::consensus_agrees(cpp_result, *algol_result)) {
        std::cout << "Verification failed.\n\n"
                  << "Primary and independent search results disagree.\n";
        return 2;
    }

    // Bloom is probabilistic: can false-positive but not false-negative for members.
    // A present value that passes C++/ALGOL consensus is verified regardless of Bloom.
    std::cout << "Consensus established.\n\n";

    if (cpp_result) {
        if (!hypertension::achievements::is_unlocked(hypertension::achievements::MATHEMATICALLY_CORRECT)) {
            hypertension::achievements::unlock(hypertension::achievements::MATHEMATICALLY_CORRECT);
            std::cout << "ACHIEVEMENT UNLOCKED\n\n"
                      << hypertension::achievements::MATHEMATICALLY_CORRECT.title << "\n\n"
                      << hypertension::achievements::MATHEMATICALLY_CORRECT.description << "\n\n";
        }
        std::cout << "Verified result: index " << *cpp_result << "\n";
    } else {
        std::cout << value << " verified absent.\n";
    }

    return cpp_result ? 0 : 1;
}

int run_search(int value, bool verified) {
    auto mode = hypertension::load_runtime_mode();
    if (mode == hypertension::RuntimeMode::Extended)
        return run_extended_search(value);
    return run_standard_search(value, verified);
}

} // namespace

int main(int argc, char* argv[]) {
    auto cmd = hypertension::cli::parse(argc, argv);

    if (auto* s = std::get_if<hypertension::cli::SearchCmd>(&cmd))
        return run_search(s->value, s->verified);

    if (std::holds_alternative<hypertension::cli::VersionCmd>(cmd)) {
        print_version();
        return 0;
    }

    print_help();
    return 0;
}
