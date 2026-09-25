#include "mode.hpp"
#include "../util/data_dir.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace hypertension {

namespace {

auto state_path() -> std::filesystem::path {
    return user_data_dir() / "mode";
}

} // namespace

auto load_runtime_mode() -> RuntimeMode {
    // Open the file.
    std::ifstream f(state_path());
    if (!f) return RuntimeMode::Standard;
    std::string val;
    f >> val;
    return (val == "extended") ? RuntimeMode::Extended : RuntimeMode::Standard;
}

// The state file contains exactly one line: the string "extended" or
// "standard", followed by a newline. Any value other than "extended"
// is treated as Standard mode. The file is never locked; concurrent
// writes are not supported and must not occur.
void save_runtime_mode(RuntimeMode mode) {
    auto path = state_path();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path);
    f << (mode == RuntimeMode::Extended ? "extended" : "standard") << '\n';
}

} // namespace hypertension
