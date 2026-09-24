#pragma once

#include <cstdlib>
#include <filesystem>

namespace hypertension {

[[nodiscard]] inline auto user_data_dir() -> std::filesystem::path {
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg)
        return std::filesystem::path(xdg) / "hypertension";
    if (const char* home = std::getenv("HOME"); home && *home)
        return std::filesystem::path(home) / ".local" / "share" / "hypertension";
    return std::filesystem::path("/tmp") / "hypertension";
}

} // namespace hypertension
