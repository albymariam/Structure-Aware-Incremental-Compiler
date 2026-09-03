#include "incppbuild/normalization/Canonicalizer.hpp"
#include <sstream>

void Canonicalizer::reset() {
    symbolMap.clear();
    varCounter = 0;
}

std::string Canonicalizer::getCanonicalName(const std::string& originalName) {
    if (originalName.empty()) return "";
    if (symbolMap.find(originalName) == symbolMap.end()) {
        symbolMap[originalName] = "$v" + std::to_string(varCounter++);
    }
    return symbolMap[originalName];
}

std::string Canonicalizer::canonicalizeNode(const std::shared_ptr<ASTNode>& node) {
    if (!node) return "";

    std::ostringstream ss;
    ss << "(";

    switch (node->type) {
        case ASTNodeType::FunctionDecl:
            ss << "FunctionDecl:" << node->name << " Type:" << node->typeName;
            break;
        case ASTNodeType::ParamDecl:
            ss << "ParamDecl:" << getCanonicalName(node->name) << " Type:" << node->typeName;
            break;
        case ASTNodeType::VarDecl:
            ss << "VarDecl:" << getCanonicalName(node->name) << " Type:" << node->typeName;
            break;
        case ASTNodeType::CompoundStmt:
            ss << "Block";
            break;
        case ASTNodeType::ReturnStmt:
            ss << "Return";
            break;
        case ASTNodeType::IfStmt:
            ss << "If";
            break;
        case ASTNodeType::BinaryOperator:
            ss << "Op:" << node->value;
            break;
        case ASTNodeType::VarRef:
            ss << "Ref:" << getCanonicalName(node->name);
            break;
        case ASTNodeType::Literal:
            ss << "Val:" << node->value;
            break;
        case ASTNodeType::CallExpr:
            ss << "Call:" << node->name;
            break;
    }

    for (const auto& child : node->children) {
        ss << " " << canonicalizeNode(child);
    }

    ss << ")";
    return ss.str();
}

std::string Canonicalizer::canonicalize(const FunctionAST& funcAST) {
    reset();
    std::ostringstream ss;
    ss << "FuncSignature[" << funcAST.functionName << ":" << funcAST.returnType << "(";
    for (size_t i = 0; i < funcAST.paramTypes.size(); ++i) {
        ss << funcAST.paramTypes[i] << (i + 1 < funcAST.paramTypes.size() ? "," : "");
    }
    ss << ")]Body" << canonicalizeNode(funcAST.rootNode);
    return ss.str();
}
