#ifndef AST_NODE_HPP
#define AST_NODE_HPP

#include <string>
#include <vector>
#include <memory>

enum class ASTNodeType {
    FunctionDecl,
    ParamDecl,
    VarDecl,
    CompoundStmt,
    DeclStmt,
    ReturnStmt,
    IfStmt,
    ElseStmt,
    ForStmt,
    WhileStmt,
    DoStmt,
    BinaryOperator,
    UnaryOperator,
    ConditionalOperator,
    CastExpr,
    VarRef,
    Literal,
    CallExpr,
    UnknownExpr
};

struct ASTNode {
    ASTNodeType type;
    std::string name;           // Original identifier name (e.g. "username", "price")
    std::string typeName;       // Data type (e.g. "int", "double", "std::string")
    std::string value;          // Literal value or operator (e.g. "+", "*", "100")
    std::vector<std::shared_ptr<ASTNode>> children;

    ASTNode(ASTNodeType t, const std::string& n = "", const std::string& typeN = "", const std::string& val = "")
        : type(t), name(n), typeName(typeN), value(val) {}

    void addChild(std::shared_ptr<ASTNode> child) {
        if (child) children.push_back(child);
    }
};

struct FunctionAST {
    std::string functionName;
    std::string returnType;
    std::vector<std::string> paramTypes;
    std::string visibility = "unspecified";
    std::shared_ptr<ASTNode> rootNode;
};

#endif // AST_NODE_HPP
