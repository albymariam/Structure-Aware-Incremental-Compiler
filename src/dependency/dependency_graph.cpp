#include "incppbuild/dependency/dependency_graph.hpp"

#include <queue>
#include <unordered_set>

namespace incppbuild {

void DependencyGraph::add_dependency(std::string dependent, std::string dependency) {
    dependents_[std::move(dependency)].push_back(std::move(dependent));
}

std::vector<std::string>
DependencyGraph::affected_by(const std::vector<std::string>& changed_units) const {
    std::queue<std::string> pending;
    std::unordered_set<std::string> seen;
    for (const auto& unit : changed_units) {
        if (seen.insert(unit).second) pending.push(unit);
    }

    std::vector<std::string> result;
    while (!pending.empty()) {
        const auto unit = pending.front();
        pending.pop();
        result.push_back(unit);
        if (const auto it = dependents_.find(unit); it != dependents_.end()) {
            for (const auto& dependent : it->second) {
                if (seen.insert(dependent).second) pending.push(dependent);
            }
        }
    }
    return result;
}

}  // namespace incppbuild
