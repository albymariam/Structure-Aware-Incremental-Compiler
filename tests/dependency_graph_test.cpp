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

    const auto cosmetic = graph.affected_by_change(
        {"parser.cpp"}, incppbuild::ChangeLevel::cosmetic);
    assert(cosmetic.empty());

    const auto renamed = graph.affected_by_change(
        {"parser.cpp"}, incppbuild::ChangeLevel::local_identifier_rename);
    assert(renamed.empty());

    const auto body_change = graph.affected_by_change(
        {"parser.cpp"}, incppbuild::ChangeLevel::body_logic);
    assert(body_change.size() == 1);
    assert(body_change.front() == "parser.cpp");

    const auto interface_change = graph.affected_by_change(
        {"ast.hpp"}, incppbuild::ChangeLevel::interface_change);
    assert(interface_change.size() == 3);
    assert(std::find(interface_change.begin(), interface_change.end(), "driver.cpp") != interface_change.end());
}
