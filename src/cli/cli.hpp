#pragma once

#include <optional>
#include <variant>

namespace hypertension::cli {

struct HelpCmd {};
struct VersionCmd {};
struct SearchCmd { int value; bool verified; };

using Command = std::variant<HelpCmd, VersionCmd, SearchCmd>;

[[nodiscard]] auto parse(int argc, char* argv[]) -> Command;

} // namespace hypertension::cli
