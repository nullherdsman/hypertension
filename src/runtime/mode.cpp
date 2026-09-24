#include "mode.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace hypertension {

namespace {

auto state_path() -> std::filesystem::path {
    std::filesystem::path base;
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg) {
        base = xdg;
    } else if (const char* home = std::getenv("HOME"); home && *home) {
        base = std::filesystem::path(home) / ".local" / "share";
    } else {
        base = "/tmp";
    }
    return base / "hypertension" / "mode";
}

} // namespace

auto load_runtime_mode() -> RuntimeMode {
    std::ifstream f(state_path());
    if (!f) return RuntimeMode::Standard;
    std::string val;
    f >> val;
    return (val == "extended") ? RuntimeMode::Extended : RuntimeMode::Standard;
}

void save_runtime_mode(RuntimeMode mode) {
    auto path = state_path();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path);
    f << (mode == RuntimeMode::Extended ? "extended" : "standard") << '\n';
}

} // namespace hypertension
