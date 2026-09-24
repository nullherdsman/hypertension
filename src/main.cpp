#include "achievements/achievements.hpp"
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
              << "─────────────"
                 "─────────────"
                 "───\n\n";

    hypertension::Timer timer;
    auto result = hypertension::find_index<int>(DATASET, value);
    auto ns = timer.elapsed_ns();

    std::string error;
    auto bloom_result = hypertension::math::verify_membership(value, error);

    if (!bloom_result.has_value()) {
        std::cerr << "error: Brainfuck membership filter failed: " << error << '\n';
        return 2;
    }

    std::cout << "✓ C++ linear search: ";
    if (result)
        std::cout << value << " at index " << *result << " (" << ns << " ns)\n";
    else
        std::cout << "not found (" << ns << " ns)\n";

    std::cout << "✓ Brainfuck membership filter: "
              << (*bloom_result ? "confirmed" : "not a member") << "\n\n";

    bool confirmed = result.has_value() && *bloom_result;

    if (confirmed) {
        std::cout << "Verification confidence: 76%\n\n";

        if (!hypertension::achievements::is_unlocked(hypertension::achievements::MATHEMATICALLY_CORRECT)) {
            hypertension::achievements::unlock(hypertension::achievements::MATHEMATICALLY_CORRECT);
            std::cout << "ACHIEVEMENT UNLOCKED\n\n"
                      << hypertension::achievements::MATHEMATICALLY_CORRECT.title << "\n\n"
                      << hypertension::achievements::MATHEMATICALLY_CORRECT.description << "\n\n";
        }

        std::cout << "RULE 2\n\n"
                  << "Two implementations agreeing is not consensus.\n\n"
                  << "A third implementation is required.\n\n";
    } else {
        std::cout << value << (result ? " found but membership filter inconclusive."
                                      : " not in dataset.") << "\n";
    }

    return result ? 0 : 1;
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
