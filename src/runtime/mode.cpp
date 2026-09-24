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
