#pragma once

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

private:
    std::unordered_map<std::string, std::vector<std::string>> dependents_;
};

}  // namespace incppbuild
