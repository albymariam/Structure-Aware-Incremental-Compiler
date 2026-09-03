#include "incppbuild/fingerprint/fingerprint.hpp"
#include "incppbuild/core/change.hpp"
#include "incppbuild/normalization/Canonicalizer.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace {

FunctionAST make_binary_function(const std::string& name, const std::string& left,
                                 const std::string& right, const std::string& local,
                                 const std::string& operation,
                                 const std::string& return_type = "int") {
    FunctionAST function;
    function.functionName = name;
    function.returnType = return_type;
    function.paramTypes = {"int", "int"};

    auto root = std::make_shared<ASTNode>(ASTNodeType::FunctionDecl, name, return_type);
    root->addChild(std::make_shared<ASTNode>(ASTNodeType::ParamDecl, left, "int"));
    root->addChild(std::make_shared<ASTNode>(ASTNodeType::ParamDecl, right, "int"));

    auto body = std::make_shared<ASTNode>(ASTNodeType::CompoundStmt, "Block");
    auto declaration = std::make_shared<ASTNode>(ASTNodeType::DeclStmt);
    auto variable = std::make_shared<ASTNode>(ASTNodeType::VarDecl, local, "int");
    auto expression = std::make_shared<ASTNode>(ASTNodeType::BinaryOperator, "", "", operation);
    expression->addChild(std::make_shared<ASTNode>(ASTNodeType::VarRef, left));
    expression->addChild(std::make_shared<ASTNode>(ASTNodeType::VarRef, right));
    variable->addChild(expression);
    declaration->addChild(variable);
    body->addChild(declaration);

    auto returned = std::make_shared<ASTNode>(ASTNodeType::ReturnStmt);
    returned->addChild(std::make_shared<ASTNode>(ASTNodeType::VarRef, local));
    body->addChild(returned);
    root->addChild(body);
    function.rootNode = root;
    return function;
}

}  // namespace

int main() {
    const auto first = make_binary_function("add", "a", "b", "result", "+");
    const auto renamed = make_binary_function("add", "x", "y", "value", "+");
    const auto changed = make_binary_function("add", "a", "b", "result", "*");
    const auto interface_changed = make_binary_function("add", "a", "b", "result", "+", "double");
    const auto second = make_binary_function("multiply", "left", "right", "product", "*");

    Canonicalizer canonicalizer;
    assert(canonicalizer.canonicalize(first) == canonicalizer.canonicalize(renamed));

    const auto first_fingerprint = Fingerprinter::fingerprintFunction(first);
    const auto renamed_fingerprint = Fingerprinter::fingerprintFunction(renamed);
    const auto changed_fingerprint = Fingerprinter::fingerprintFunction(changed);
    assert(first_fingerprint.hash == renamed_fingerprint.hash);
    assert(first_fingerprint.hash != changed_fingerprint.hash);
    assert(first_fingerprint.interfaceHash == renamed_fingerprint.interfaceHash);
    assert(first_fingerprint.implementationHash == renamed_fingerprint.implementationHash);
    assert(first_fingerprint.interfaceHash == changed_fingerprint.interfaceHash);
    assert(first_fingerprint.implementationHash != changed_fingerprint.implementationHash);

    const auto interface_changed_fingerprint = Fingerprinter::fingerprintFunction(interface_changed);
    assert(first_fingerprint.interfaceHash != interface_changed_fingerprint.interfaceHash);

    const auto forward = Fingerprinter::fingerprintTranslationUnit("math.cpp", {first, second});
    const auto reverse = Fingerprinter::fingerprintTranslationUnit("math.cpp", {second, first});
    assert(forward.hash == reverse.hash);
    assert(forward.functions.size() == 2);
    assert(forward.functions[0].functionName == "add");

    const auto unchanged = Fingerprinter::fingerprintTranslationUnit("math.cpp", {first, second});
    const auto logic_changed = Fingerprinter::fingerprintTranslationUnit("math.cpp", {changed, second});
    const auto signature_changed = Fingerprinter::fingerprintTranslationUnit(
        "math.cpp", {interface_changed, second});
    const auto removed = Fingerprinter::fingerprintTranslationUnit("math.cpp", {first});

    const auto logic_changes = incppbuild::analyze_function_changes(forward, logic_changed);
    assert(logic_changes.size() == 1);
    assert(logic_changes.front().functionName == "add");
    assert(logic_changes.front().level == incppbuild::ChangeLevel::body_logic);

    const auto signature_changes = incppbuild::analyze_function_changes(forward, signature_changed);
    assert(signature_changes.size() == 1);
    assert(signature_changes.front().level == incppbuild::ChangeLevel::interface_change);
    assert(incppbuild::invalidates_dependents(signature_changes.front().level));

    const auto removal_changes = incppbuild::analyze_function_changes(forward, removed);
    assert(removal_changes.size() == 1);
    assert(removal_changes.front().functionName == "multiply");
    assert(removal_changes.front().level == incppbuild::ChangeLevel::interface_change);

    assert(incppbuild::analyze_function_changes(forward, unchanged).empty());

    const auto impact = incppbuild::analyze_function_impact(forward, logic_changed);
    assert(impact.size() == 2);
    assert(impact[0].functionName == "add");
    assert(impact[0].level == incppbuild::ChangeLevel::body_logic);
    assert(impact[1].functionName == "multiply");
    assert(impact[1].level == incppbuild::ChangeLevel::no_change);
}
