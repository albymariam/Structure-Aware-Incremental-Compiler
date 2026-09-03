#include "incppbuild/dependency/dependency_graph.hpp"

#include <algorithm>
#include <cassert>

int main() {
    incppbuild::DependencyGraph graph;
    graph.add_dependency("parser.cpp", "ast.hpp");
    graph.add_dependency("driver.cpp", "parser.cpp");

    const auto affected = graph.affected_by({"ast.hpp"});
    assert(affected.size() == 3);
    assert(std::find(affected.begin(), affected.end(), "ast.hpp") != affected.end());
    assert(std::find(affected.begin(), affected.end(), "parser.cpp") != affected.end());
    assert(std::find(affected.begin(), affected.end(), "driver.cpp") != affected.end());
}
