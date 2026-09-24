#pragma once

namespace hypertension {

enum class RuntimeMode { Standard, Extended };

[[nodiscard]] auto load_runtime_mode() -> RuntimeMode;
void save_runtime_mode(RuntimeMode mode);

} // namespace hypertension
