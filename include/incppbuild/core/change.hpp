#pragma once

#include "incppbuild/fingerprint/fingerprint.hpp"

#include <string_view>
#include <string>
#include <vector>

namespace incppbuild {

enum class ChangeLevel {
    no_change = 0,
    cosmetic = 1,
    local_identifier_rename = 2,
    body_logic = 3,
    interface_change = 4,

    NO_CHANGE = no_change,
    COSMETIC_CHANGE = cosmetic,
    LOCAL_RENAME = local_identifier_rename,
    BODY_LOGIC_CHANGE = body_logic,
    INTERFACE_CHANGE = interface_change,
};

struct FunctionChange {
    std::string functionName;
    ChangeLevel level = ChangeLevel::no_change;
    std::string oldFingerprint;
    std::string newFingerprint;
};

[[nodiscard]] std::string_view to_string(ChangeLevel level) noexcept;
[[nodiscard]] bool requires_recompilation(ChangeLevel level) noexcept;
[[nodiscard]] bool invalidates_dependents(ChangeLevel level) noexcept;

[[nodiscard]] ChangeLevel classify_change(
    const FunctionFingerprint* oldFunction,
    const FunctionFingerprint* newFunction) noexcept;

[[nodiscard]] std::vector<FunctionChange> analyze_function_changes(
    const TranslationUnitFingerprint& oldUnit,
    const TranslationUnitFingerprint& newUnit);

// Returns one row for every function in either snapshot, including unchanged functions.
[[nodiscard]] std::vector<FunctionChange> analyze_function_impact(
    const TranslationUnitFingerprint& oldUnit,
    const TranslationUnitFingerprint& newUnit);

}  // namespace incppbuild
