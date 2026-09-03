#pragma once

#include <string_view>

namespace incppbuild {

enum class ChangeLevel {
    no_change = 0,
    cosmetic = 1,
    local_identifier_rename = 2,
    body_logic = 3,
    interface_change = 4,
};

[[nodiscard]] std::string_view to_string(ChangeLevel level) noexcept;
[[nodiscard]] bool requires_recompilation(ChangeLevel level) noexcept;
[[nodiscard]] bool invalidates_dependents(ChangeLevel level) noexcept;

}  // namespace incppbuild
