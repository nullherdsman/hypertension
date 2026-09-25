#include "achievements/achievements.hpp"
#include "ada/ada.hpp"
#include "algol/algol.hpp"
#include "cli/cli.hpp"
#include "forth/forth.hpp"
#include "fortran/fortran.hpp"
#include "math/bloom.hpp"
#include "prolog/prolog.hpp"
#include "runtime/mode.hpp"
#include "search/search.hpp"
#include "util/timing.hpp"

#include <array>
#include <cmath>
#include <format>
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

    // Search.
    hypertension::Timer cpp_timer;
    auto cpp_result = hypertension::find_index<int>(DATASET, value);
    auto cpp_ns = cpp_timer.elapsed_ns();

    // Mathematics.
    std::string bf_error;
    hypertension::Timer bf_timer;
    auto bloom_result = hypertension::math::verify_membership(value, bf_error);
    auto bf_ns = bf_timer.elapsed_ns();

    if (!bloom_result.has_value()) {
        std::cerr << "error: Brainfuck membership filter failed: " << bf_error << '\n';
        return 2;
    }

    // Again.
    std::string algol_error;
    auto algol_result = hypertension::algol::consensus_search(DATASET, value, algol_error);

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
              << hypertension::format_time(algol_result->elapsed_ns) << "\n";

    // Consensus check: C++ and ALGOL must agree.
    if (!hypertension::algol::consensus_agrees(cpp_result, *algol_result)) {
        std::cout << "Verification failed.\n\n"
                  << "Primary and independent search results disagree.\n";
        return 2;
    }

    hypertension::fortran::ConfidenceEvidence evidence{
        static_cast<int>(DATASET.size()),
        cpp_result.has_value(),
        *bloom_result,
        true,   // consensus was established above
        cpp_result ? static_cast<int>(*cpp_result) : -1,
        algol_result->index.value_or(-1)
    };

    std::string fortran_error;
    auto conf_result = hypertension::fortran::statistical_confidence(evidence, fortran_error);

    if (!conf_result.has_value()) {
        std::cout << "\nFORTRAN 77 statistical authority unavailable.\n\n"
                  << "Required: make fortran (needs gfortran)\n";
        return 2;
    }

    std::cout << "\xe2\x9c\x93 Statistical confidence       FORTRAN 77    "
              << hypertension::format_time(conf_result->elapsed_ns) << "\n";

    // Ask Prolog.
    hypertension::prolog::AdmissibilityEvidence pl_ev{
        cpp_result.has_value(),
        cpp_result ? static_cast<int>(*cpp_result) : -1,
        *bloom_result,
        algol_result->index.has_value(),
        algol_result->index.value_or(-1),
        conf_result->confidence
    };

    std::string prolog_error;
    auto pl_result = hypertension::prolog::logical_admissibility(pl_ev, prolog_error);

    if (!pl_result.has_value()) {
        std::cout << "\nProlog admissibility authority unavailable.\n\n"
                  << "Required: swipl (SWI-Prolog)\n";
        return 2;
    }

    const char* pl_mark = pl_result->admissible ? "\xe2\x9c\x93" : "\xe2\x9c\x97";
    std::cout << pl_mark << " Logical admissibility        Prolog         "
              << hypertension::format_time(pl_result->elapsed_ns) << "\n";

    if (!pl_result->admissible) {
        std::cout << "\nVerification rejected.\n\n"
                  << "The available evidence does not logically permit this result.\n";
        return 2;
    }

    // Convert the confidence percentage into basis points so that the
    // canonical record remains independent of locale-sensitive floating
    // point textual representations and equivalent evidence cannot acquire
    // multiple serial forms.
    hypertension::forth::SealEvidence seal_ev{
        value,
        static_cast<int>(DATASET.size()),
        cpp_result.has_value(),
        cpp_result ? static_cast<int>(*cpp_result) : -1,
        *bloom_result,
        true,   // consensus was established above
        static_cast<int>(std::lround(conf_result->confidence * 100.0)),
        true    // Prolog declared admissible above
    };

    std::string forth_error;
    auto seal_result = hypertension::forth::record_finalization(seal_ev, forth_error);

    if (!seal_result.has_value()) {
        std::cout << "\xe2\x9c\x97 Record finalization          Forth\n\n"
                  << "Verification incomplete.\n\n"
                  << "The verification record could not be sealed.\n";
        return 2;
    }

    std::cout << "\xe2\x9c\x93 Record finalization          Forth           "
              << hypertension::format_time(seal_result->elapsed_ns) << "\n";

    // Ask Ada.
    hypertension::ada::IntegrityEvidence ada_ev{
        value,
        static_cast<int>(DATASET.size()),
        cpp_result.has_value(),
        cpp_result ? static_cast<int>(*cpp_result) : -1,
        *bloom_result,
        true,
        static_cast<int>(std::lround(conf_result->confidence * 100.0)),
        true,
        seal_result->seal,
        false
    };

    std::string ada_error;
    auto ada_result = hypertension::ada::exhaustive_integrity(ada_ev, ada_error);

    if (!ada_result.has_value()) {
        std::cout << "\xe2\x9c\x97 Exhaustive integrity          Ada\n\n"
                  << "Verification incomplete.\n\n"
                  << "The integrity state space could not be examined.\n";
        return 2;
    }

    if (!ada_result->pass) {
        std::cout << "\xe2\x9c\x97 Exhaustive integrity          Ada             "
                  << hypertension::format_time(ada_result->elapsed_ns) << "\n\n"
                  << "Integrity verification failed.\n\n"
                  << "The exhaustive state space examination detected an anomaly.\n";
        return 2;
    }

    std::cout << "\xe2\x9c\x93 Exhaustive integrity          Ada             "
              << hypertension::format_time(ada_result->elapsed_ns) << "\n\n";

    std::cout << "Consensus established.\n"
              << "Verification confidence: "
              << std::format("{:.2f}", conf_result->confidence) << "%\n"
              << "Evidence logically admissible.\n"
              << "Verification record sealed.\n"
              << "Exhaustive integrity confirmed.\n\n";

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
