#pragma once

#include "incppbuild/core/change.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace incppbuild {

class DependencyGraph {
public:
    // Records that `dependent` must be rebuilt when `dependency` changes.
    void add_dependency(std::string dependent, std::string dependency);

    [[nodiscard]] std::vector<std::string>
    affected_by(const std::vector<std::string>& changed_units) const;

    // Applies the project's change policy to a set of changed source units.
    // Interface changes traverse the reverse dependency graph; body-only
    // changes rebuild only their own translation unit.
    [[nodiscard]] std::vector<std::string>
    affected_by_change(const std::vector<std::string>& changed_units,
                       ChangeLevel level) const;

private:
    std::unordered_map<std::string, std::vector<std::string>> dependents_;
};

}  // namespace incppbuild
