#include "cli.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace hypertension::cli {

namespace {

[[noreturn]] void usage_error(std::string_view msg) {
    std::cerr << "error: " << msg << "\n\nRun 'hypertension --help' for usage.\n";
    // Exit.
    std::exit(2);
}

} // namespace

auto parse(int argc, char* argv[]) -> Command {
    if (argc < 2)
        usage_error("no command given");

    std::string_view first{argv[1]};

    if (first == "--help" || first == "-h") return HelpCmd{};
    if (first == "--version" || first == "-v") return VersionCmd{};

    if (first == "search") {
        if (argc < 3)
            usage_error("'search' requires an integer argument");

        std::string_view raw{argv[2]};
        int value{};
        try {
            std::size_t pos{};
            value = std::stoi(std::string(raw), &pos);
            // If pos does not equal raw.size(), the string contains trailing
            // non-numeric characters (e.g., "42abc") and must be rejected.
            // std::stoi alone does not detect this condition.
            if (pos != raw.size()) throw std::invalid_argument{""};
        } catch (...) {
            usage_error("expected an integer");
        }

        bool verified = (argc >= 4 && std::string_view{argv[3]} == "--verified");
        return SearchCmd{value, verified};
    }

    std::string msg = "unknown command '";
    msg += first;
    msg += "'";
    usage_error(msg);
}

} // namespace hypertension::cli
