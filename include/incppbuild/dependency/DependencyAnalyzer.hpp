#pragma once

#include "incppbuild/core/CompilationDatabase.hpp"
#include "incppbuild/dependency/dependency_graph.hpp"

#include <filesystem>
#include <optional>
#include <vector>

namespace incppbuild {

struct DependencyEdge {
    std::filesystem::path dependent;
    std::filesystem::path dependency;
};

struct DependencyAnalysis {
    std::filesystem::path translationUnit;
    std::vector<DependencyEdge> edges;
};

// Clang-preprocessor-backed dependency discovery. It observes resolved include
// directives under the same arguments used to compile each translation unit.
class DependencyAnalyzer {
public:
    [[nodiscard]] static std::optional<DependencyAnalysis> analyzeFile(
        const CompileCommand& command);
    [[nodiscard]] static std::vector<DependencyAnalysis> analyze(
        const CompilationDatabase& database);

    static void addToGraph(const DependencyAnalysis& analysis, DependencyGraph& graph);
};

}  // namespace incppbuild
