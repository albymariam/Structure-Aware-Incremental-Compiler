#include "incppbuild/ast/ClangASTParser.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/SmallString.h>

#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

namespace incppbuild {
namespace {

class FunctionVisitor final : public clang::RecursiveASTVisitor<FunctionVisitor> {
public:
    explicit FunctionVisitor(std::vector<FunctionAST>& functions) : functions_(functions) {}

    bool VisitFunctionDecl(clang::FunctionDecl* declaration) {
        if (!declaration->hasBody() || !declaration->isThisDeclarationADefinition() ||
            declaration->isImplicit() || !declaration->getASTContext().getSourceManager()
                .isWrittenInMainFile(declaration->getLocation())) {
            return true;
        }

        FunctionAST function;
        function.functionName = declaration->getQualifiedNameAsString();
        function.returnType = declaration->getReturnType().getCanonicalType().getAsString();
        switch (declaration->getAccess()) {
        case clang::AS_public: function.visibility = "public"; break;
        case clang::AS_protected: function.visibility = "protected"; break;
        case clang::AS_private: function.visibility = "private"; break;
        case clang::AS_none: function.visibility = "global"; break;
        }

        auto root = std::make_shared<ASTNode>(
            ASTNodeType::FunctionDecl, function.functionName, function.returnType);
        for (const clang::ParmVarDecl* parameter : declaration->parameters()) {
            const auto type = parameter->getType().getCanonicalType().getAsString();
            function.paramTypes.push_back(type);
            root->addChild(std::make_shared<ASTNode>(
                ASTNodeType::ParamDecl, parameter->getNameAsString(), type));
        }

        root->addChild(build_statement(declaration->getBody()));
        function.rootNode = std::move(root);
        functions_.push_back(std::move(function));
        return true;
    }

private:
    static std::shared_ptr<ASTNode> build_expression(const clang::Expr* expression) {
        if (!expression) return nullptr;
        expression = expression->IgnoreParenImpCasts();

        if (const auto* binary = llvm::dyn_cast<clang::BinaryOperator>(expression)) {
            auto node = std::make_shared<ASTNode>(ASTNodeType::BinaryOperator, "", "",
                binary->getOpcodeStr().str());
            node->addChild(build_expression(binary->getLHS()));
            node->addChild(build_expression(binary->getRHS()));
            return node;
        }
        if (const auto* unary = llvm::dyn_cast<clang::UnaryOperator>(expression)) {
            auto node = std::make_shared<ASTNode>(ASTNodeType::UnaryOperator, "", "",
                unary->getOpcodeStr(unary->getOpcode()).str());
            node->addChild(build_expression(unary->getSubExpr()));
            return node;
        }
        if (const auto* call = llvm::dyn_cast<clang::CallExpr>(expression)) {
            std::string name = "<indirect>";
            if (const auto* callee = call->getDirectCallee()) name = callee->getQualifiedNameAsString();
            auto node = std::make_shared<ASTNode>(ASTNodeType::CallExpr, name);
            for (const clang::Expr* argument : call->arguments()) node->addChild(build_expression(argument));
            return node;
        }
        if (const auto* reference = llvm::dyn_cast<clang::DeclRefExpr>(expression)) {
            return std::make_shared<ASTNode>(ASTNodeType::VarRef,
                reference->getDecl()->getNameAsString(),
                reference->getType().getCanonicalType().getAsString());
        }
        if (const auto* literal = llvm::dyn_cast<clang::IntegerLiteral>(expression)) {
            llvm::SmallString<32> text;
            literal->getValue().toString(text, 10, true);
            return std::make_shared<ASTNode>(ASTNodeType::Literal, "", "", text.str().str());
        }
        if (const auto* literal = llvm::dyn_cast<clang::FloatingLiteral>(expression)) {
            llvm::SmallString<32> text;
            literal->getValue().toString(text);
            return std::make_shared<ASTNode>(ASTNodeType::Literal, "", "", text.str().str());
        }
        if (const auto* literal = llvm::dyn_cast<clang::StringLiteral>(expression)) {
            return std::make_shared<ASTNode>(ASTNodeType::Literal, "", "", literal->getString().str());
        }
        if (const auto* literal = llvm::dyn_cast<clang::CharacterLiteral>(expression)) {
            return std::make_shared<ASTNode>(ASTNodeType::Literal, "", "", std::to_string(literal->getValue()));
        }
        if (const auto* conditional = llvm::dyn_cast<clang::ConditionalOperator>(expression)) {
            auto node = std::make_shared<ASTNode>(ASTNodeType::ConditionalOperator);
            node->addChild(build_expression(conditional->getCond()));
            node->addChild(build_expression(conditional->getTrueExpr()));
            node->addChild(build_expression(conditional->getFalseExpr()));
            return node;
        }
        if (const auto* cast = llvm::dyn_cast<clang::CastExpr>(expression)) {
            auto node = std::make_shared<ASTNode>(ASTNodeType::CastExpr, "",
                cast->getType().getCanonicalType().getAsString());
            node->addChild(build_expression(cast->getSubExpr()));
            return node;
        }
        return std::make_shared<ASTNode>(ASTNodeType::UnknownExpr,
            expression->getStmtClassName(), expression->getType().getCanonicalType().getAsString());
    }

    static std::shared_ptr<ASTNode> build_statement(const clang::Stmt* statement) {
        if (!statement) return nullptr;
        if (const auto* compound = llvm::dyn_cast<clang::CompoundStmt>(statement)) {
            auto node = std::make_shared<ASTNode>(ASTNodeType::CompoundStmt, "Block");
            for (const clang::Stmt* child : compound->body()) node->addChild(build_statement(child));
            return node;
        }
        if (const auto* declaration = llvm::dyn_cast<clang::DeclStmt>(statement)) {
            auto node = std::make_shared<ASTNode>(ASTNodeType::DeclStmt);
            for (const clang::Decl* declaration_node : declaration->decls()) {
                if (const auto* variable = llvm::dyn_cast<clang::VarDecl>(declaration_node)) {
                    auto variable_node = std::make_shared<ASTNode>(ASTNodeType::VarDecl,
                        variable->getNameAsString(), variable->getType().getCanonicalType().getAsString());
                    variable_node->addChild(build_expression(variable->getInit()));
                    node->addChild(variable_node);
                }
            }
            return node;
        }
        if (const auto* returned = llvm::dyn_cast<clang::ReturnStmt>(statement)) {
            auto node = std::make_shared<ASTNode>(ASTNodeType::ReturnStmt);
            node->addChild(build_expression(returned->getRetValue()));
            return node;
        }
        if (const auto* conditional = llvm::dyn_cast<clang::IfStmt>(statement)) {
            auto node = std::make_shared<ASTNode>(ASTNodeType::IfStmt);
            node->addChild(build_expression(conditional->getCond()));
            node->addChild(build_statement(conditional->getThen()));
            if (conditional->getElse()) {
                auto else_node = std::make_shared<ASTNode>(ASTNodeType::ElseStmt);
                else_node->addChild(build_statement(conditional->getElse()));
                node->addChild(else_node);
            }
            return node;
        }
        if (const auto* loop = llvm::dyn_cast<clang::ForStmt>(statement)) {
            auto node = std::make_shared<ASTNode>(ASTNodeType::ForStmt);
            node->addChild(build_statement(loop->getInit()));
            node->addChild(build_expression(loop->getCond()));
            node->addChild(build_expression(loop->getInc()));
            node->addChild(build_statement(loop->getBody()));
            return node;
        }
        if (const auto* loop = llvm::dyn_cast<clang::WhileStmt>(statement)) {
            auto node = std::make_shared<ASTNode>(ASTNodeType::WhileStmt);
            node->addChild(build_expression(loop->getCond()));
            node->addChild(build_statement(loop->getBody()));
            return node;
        }
        if (const auto* loop = llvm::dyn_cast<clang::DoStmt>(statement)) {
            auto node = std::make_shared<ASTNode>(ASTNodeType::DoStmt);
            node->addChild(build_statement(loop->getBody()));
            node->addChild(build_expression(loop->getCond()));
            return node;
        }
        if (const auto* expression = llvm::dyn_cast<clang::Expr>(statement)) return build_expression(expression);
        return std::make_shared<ASTNode>(ASTNodeType::UnknownExpr, statement->getStmtClassName());
    }

    std::vector<FunctionAST>& functions_;
};

class CollectingConsumer final : public clang::ASTConsumer {
public:
    explicit CollectingConsumer(std::vector<FunctionAST>& functions) : visitor_(functions) {}

    void HandleTranslationUnit(clang::ASTContext& context) override {
        visitor_.TraverseDecl(context.getTranslationUnitDecl());
    }

private:
    FunctionVisitor visitor_;
};

class CollectingAction final : public clang::ASTFrontendAction {
public:
    explicit CollectingAction(std::vector<FunctionAST>& functions) : functions_(functions) {}

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance&, llvm::StringRef) override {
        return std::make_unique<CollectingConsumer>(functions_);
    }

private:
    std::vector<FunctionAST>& functions_;
};

}  // namespace

std::vector<FunctionAST> ClangASTParser::parse_source_file(
    const std::string& file_path,
    const std::vector<std::string>& compile_arguments) {
    std::ifstream input(file_path, std::ios::binary);
    if (!input) {
        return {};
    }

    const std::string source(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    std::vector<FunctionAST> functions;

    clang::tooling::runToolOnCodeWithArgs(
        std::make_unique<CollectingAction>(functions), source, compile_arguments, file_path,
        "incppbuild");
    return functions;
}

}  // namespace incppbuild
