#include "incppbuild/core/change.hpp"

namespace incppbuild {

std::string_view to_string(ChangeLevel level) noexcept {
    switch (level) {
    case ChangeLevel::no_change: return "no change";
    case ChangeLevel::cosmetic: return "cosmetic";
    case ChangeLevel::local_identifier_rename: return "local identifier rename";
    case ChangeLevel::body_logic: return "body logic";
    case ChangeLevel::interface_change: return "interface change";
    }
    return "unknown";
}

bool requires_recompilation(ChangeLevel level) noexcept {
    return level >= ChangeLevel::body_logic;
}

bool invalidates_dependents(ChangeLevel level) noexcept {
    return level == ChangeLevel::interface_change;
}

}  // namespace incppbuild
