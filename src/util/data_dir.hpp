#pragma once

#include <cstdlib>
#include <filesystem>

namespace hypertension {

// XDG Base Directory Specification §3.1: if $XDG_DATA_HOME is set and
// non-empty, that directory is used for user-specific data files.
// Otherwise the default is $HOME/.local/share. The /tmp fallback is for
// environments where neither variable is defined; persistence across
// reboots is not guaranteed in that case and must not be assumed.
[[nodiscard]] inline auto user_data_dir() -> std::filesystem::path {
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg)
        return std::filesystem::path(xdg) / "hypertension";
    if (const char* home = std::getenv("HOME"); home && *home)
        return std::filesystem::path(home) / ".local" / "share" / "hypertension";
    return std::filesystem::path("/tmp") / "hypertension";
}

} // namespace hypertension
