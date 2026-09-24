#include "achievements.hpp"
#include "../util/data_dir.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

namespace hypertension::achievements {

namespace {

auto achievement_path(const Achievement& a) -> std::filesystem::path {
    return hypertension::user_data_dir() / "achievements" / a.id;
}

} // namespace

auto is_unlocked(const Achievement& a) -> bool {
    return std::filesystem::exists(achievement_path(a));
}

void unlock(const Achievement& a) {
    if (is_unlocked(a)) return;
    auto path = achievement_path(a);
    std::filesystem::create_directories(path.parent_path());
    auto epoch = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::ofstream{path} << epoch << '\n';
}

} // namespace hypertension::achievements
