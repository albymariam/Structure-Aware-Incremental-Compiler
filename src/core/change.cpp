#include "incppbuild/core/change.hpp"

#include <map>
#include <utility>

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

ChangeLevel classify_change(const FunctionFingerprint* oldFunction,
                            const FunctionFingerprint* newFunction) noexcept {
    if (oldFunction == nullptr || newFunction == nullptr) {
        return ChangeLevel::interface_change;
    }
    if (oldFunction->interfaceHash != newFunction->interfaceHash) {
        return ChangeLevel::interface_change;
    }
    if (oldFunction->implementationHash != newFunction->implementationHash) {
        return ChangeLevel::body_logic;
    }
    return ChangeLevel::no_change;
}

std::vector<FunctionChange> analyze_function_changes(
    const TranslationUnitFingerprint& oldUnit,
    const TranslationUnitFingerprint& newUnit) {
    std::map<std::string, std::pair<const FunctionFingerprint*, const FunctionFingerprint*>> functions;
    for (const auto& function : oldUnit.functions) {
        functions[function.functionName].first = &function;
    }
    for (const auto& function : newUnit.functions) {
        functions[function.functionName].second = &function;
    }

    std::vector<FunctionChange> changes;
    changes.reserve(functions.size());
    for (const auto& [name, fingerprints] : functions) {
        const auto level = classify_change(fingerprints.first, fingerprints.second);
        if (level == ChangeLevel::no_change) {
            continue;
        }

        FunctionChange change;
        change.functionName = name;
        change.level = level;
        if (fingerprints.first != nullptr) {
            change.oldFingerprint = fingerprints.first->hash;
        }
        if (fingerprints.second != nullptr) {
            change.newFingerprint = fingerprints.second->hash;
        }
        changes.push_back(std::move(change));
    }
    return changes;
}

std::vector<FunctionChange> analyze_function_impact(
    const TranslationUnitFingerprint& oldUnit,
    const TranslationUnitFingerprint& newUnit) {
    std::map<std::string, std::pair<const FunctionFingerprint*, const FunctionFingerprint*>> functions;
    for (const auto& function : oldUnit.functions) {
        functions[function.functionName].first = &function;
    }
    for (const auto& function : newUnit.functions) {
        functions[function.functionName].second = &function;
    }

    std::vector<FunctionChange> impact;
    impact.reserve(functions.size());
    for (const auto& [name, fingerprints] : functions) {
        FunctionChange change;
        change.functionName = name;
        change.level = classify_change(fingerprints.first, fingerprints.second);
        if (fingerprints.first != nullptr) {
            change.oldFingerprint = fingerprints.first->hash;
        }
        if (fingerprints.second != nullptr) {
            change.newFingerprint = fingerprints.second->hash;
        }
        impact.push_back(std::move(change));
    }
    return impact;
}

}  // namespace incppbuild
