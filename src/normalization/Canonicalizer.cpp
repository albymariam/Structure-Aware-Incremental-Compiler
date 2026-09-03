#include "incppbuild/normalization/Canonicalizer.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

void Canonicalizer::reset() {
    symbolMap.clear();
    varCounter = 0;
}

std::string Canonicalizer::declareCanonicalName(const std::string& originalName) {
    if (originalName.empty()) return "";
    const auto existing = symbolMap.find(originalName);
    if (existing != symbolMap.end()) {
        return existing->second;
    }
    const auto canonical = "$v" + std::to_string(varCounter++);
    symbolMap.emplace(originalName, canonical);
    return canonical;
}

std::string Canonicalizer::referenceCanonicalName(const std::string& originalName) const {
    if (const auto found = symbolMap.find(originalName); found != symbolMap.end()) {
        return found->second;
    }
    // An undeclared identifier is not a local binding. Preserve it so a change
    // to a global variable, static member, or external symbol remains visible.
    return "@" + originalName;
}

std::string Canonicalizer::canonicalizeType(const std::string& typeName) {
    std::string result;
    bool previousWasSpace = false;
    for (const unsigned char character : typeName) {
        if (std::isspace(character)) {
            previousWasSpace = !result.empty();
            continue;
        }
        if ((character == '*' || character == '&' || character == ',' || character == '>') &&
            !result.empty() && result.back() == ' ') {
            result.pop_back();
        }
        if (character == '<' && !result.empty() && result.back() == ' ') result.pop_back();
        if (previousWasSpace && !result.empty() && character != '*' && character != '&' &&
            character != ',' && character != '>' && character != '<') {
            result += ' ';
        }
        result += static_cast<char>(character);
        previousWasSpace = false;
    }
    return result;
}

std::string Canonicalizer::canonicalizeLiteral(const std::string& literal) {
    // Clang supplies semantic literal values (for example, an integer's APInt
    // value). Preserve them exactly: changing a string or character literal is
    // a behavioral change and must change the canonical representation.
    return literal;
}

std::string Canonicalizer::canonicalizeNode(const std::shared_ptr<ASTNode>& node) {
    if (!node) return "";

    std::ostringstream ss;
    ss << "(";

    switch (node->type) {
        case ASTNodeType::FunctionDecl:
            ss << "FunctionDecl:" << node->name << " Type:" << canonicalizeType(node->typeName);
            break;
        case ASTNodeType::ParamDecl:
            ss << "ParamDecl:" << declareCanonicalName(node->name)
               << " Type:" << canonicalizeType(node->typeName);
            break;
        case ASTNodeType::VarDecl:
            ss << "VarDecl:" << declareCanonicalName(node->name)
               << " Type:" << canonicalizeType(node->typeName);
            break;
        case ASTNodeType::CompoundStmt:
            ss << "Block";
            break;
        case ASTNodeType::DeclStmt:
            ss << "DeclStmt";
            break;
        case ASTNodeType::ReturnStmt:
            ss << "Return";
            break;
        case ASTNodeType::IfStmt:
            ss << "If";
            break;
        case ASTNodeType::ElseStmt:
            ss << "Else";
            break;
        case ASTNodeType::ForStmt:
            ss << "For";
            break;
        case ASTNodeType::WhileStmt:
            ss << "While";
            break;
        case ASTNodeType::DoStmt:
            ss << "Do";
            break;
        case ASTNodeType::BinaryOperator:
            ss << "Op:" << node->value;
            break;
        case ASTNodeType::UnaryOperator:
            ss << "UnaryOp:" << node->value;
            break;
        case ASTNodeType::ConditionalOperator:
            ss << "Conditional";
            break;
        case ASTNodeType::CastExpr:
            ss << "Cast:" << canonicalizeType(node->typeName);
            break;
        case ASTNodeType::VarRef:
            ss << "Ref:" << referenceCanonicalName(node->name);
            break;
        case ASTNodeType::Literal:
            ss << "Val:" << canonicalizeLiteral(node->value);
            break;
        case ASTNodeType::CallExpr:
            ss << "Call:" << node->name;
            break;
        case ASTNodeType::UnknownExpr:
            ss << "Unknown:" << node->name;
            break;
    }

    for (const auto& child : node->children) {
        ss << " " << canonicalizeNode(child);
    }

    ss << ")";
    return ss.str();
}

std::string Canonicalizer::canonicalize(const FunctionAST& funcAST) {
    return canonicalizeInterface(funcAST) + "Implementation" + canonicalizeImplementation(funcAST);
}

std::string Canonicalizer::canonicalizeInterface(const FunctionAST& funcAST) {
    std::ostringstream ss;
    ss << "Interface[Name:" << funcAST.functionName
       << " Return:" << canonicalizeType(funcAST.returnType)
       << " Visibility:" << funcAST.visibility << " Params:(";
    for (size_t i = 0; i < funcAST.paramTypes.size(); ++i) {
        ss << canonicalizeType(funcAST.paramTypes[i]) << (i + 1 < funcAST.paramTypes.size() ? "," : "");
    }
    ss << ")]";
    return ss.str();
}

std::string Canonicalizer::canonicalizeImplementation(const FunctionAST& funcAST) {
    reset();
    std::ostringstream ss;

    if (!funcAST.rootNode) return "Body()";

    // Bind parameter names before visiting the body, but omit their spelling
    // from the implementation representation.
    for (const auto& child : funcAST.rootNode->children) {
        if (child && child->type == ASTNodeType::ParamDecl) {
            declareCanonicalName(child->name);
        }
    }

    ss << "Body";
    for (const auto& child : funcAST.rootNode->children) {
        if (child && child->type != ASTNodeType::ParamDecl) {
            ss << canonicalizeNode(child);
        }
    }
    return ss.str();
}
