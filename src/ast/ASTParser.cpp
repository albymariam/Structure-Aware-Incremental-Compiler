#include "incppbuild/ast/ASTParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>

namespace {

std::shared_ptr<ASTNode> parseExpression(const std::string& expression) {
    std::string trimmed = expression;
    const auto first = trimmed.find_first_not_of(" \t");
    if (first == std::string::npos) return nullptr;
    trimmed.erase(0, first);
    const auto last = trimmed.find_last_not_of(" \t;");
    trimmed.erase(last + 1);

    for (const char* operationText : {"+", "*"}) {
        const std::string operation(operationText);
        const auto operatorPosition = trimmed.find(operation);
        if (operatorPosition == std::string::npos) continue;

        auto binary = std::make_shared<ASTNode>(
            ASTNodeType::BinaryOperator, "", "", operation);
        const auto left = trimmed.substr(0, operatorPosition);
        const auto right = trimmed.substr(operatorPosition + operation.size());

        const auto makeOperand = [](std::string operand) {
            const auto firstNonSpace = operand.find_first_not_of(" \t");
            const auto lastNonSpace = operand.find_last_not_of(" \t");
            operand = operand.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
            const bool isNumber = !operand.empty() &&
                operand.find_first_not_of("0123456789.") == std::string::npos;
            return std::make_shared<ASTNode>(
                isNumber ? ASTNodeType::Literal : ASTNodeType::VarRef,
                isNumber ? "" : operand,
                "",
                isNumber ? operand : "");
        };

        binary->addChild(makeOperand(left));
        binary->addChild(makeOperand(right));
        return binary;
    }

    return std::make_shared<ASTNode>(ASTNodeType::VarRef, trimmed);
}

}

std::vector<FunctionAST> ASTParser::parseSourceFile(const std::string& filepath) {
    std::ifstream inFile(filepath);
    if (!inFile.is_open()) {
        std::cerr << "[ASTParser] Failed to open source file: " << filepath << std::endl;
        return {};
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    return parseSourceContent(buffer.str(), filepath);
}

std::vector<FunctionAST> ASTParser::parseSourceContent(const std::string& content, const std::string& filename) {
    std::vector<FunctionAST> functions;

    std::regex funcRegex(R"(([a-zA-Z0-9_:<>&*]+)\s+([a-zA-Z0-9_:]+)\s*\(([^)]*)\)\s*\{([^}]*)\})");
    auto words_begin = std::sregex_iterator(content.begin(), content.end(), funcRegex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        std::string retType = match[1].str();
        std::string funcName = match[2].str();
        std::string paramStr = match[3].str();
        std::string bodyStr = match[4].str();

        FunctionAST fAst;
        fAst.functionName = funcName;
        fAst.returnType = retType;

        auto root = std::make_shared<ASTNode>(ASTNodeType::FunctionDecl, funcName, retType);

        std::stringstream pss(paramStr);
        std::string param;
        while (std::getline(pss, param, ',')) {
            param.erase(0, param.find_first_not_of(" \t"));
            param.erase(param.find_last_not_of(" \t") + 1);
            if (param.empty()) continue;

            size_t lastSpace = param.find_last_of(" \t&*");
            std::string pType = (lastSpace != std::string::npos) ? param.substr(0, lastSpace + 1) : "int";
            std::string pName = (lastSpace != std::string::npos) ? param.substr(lastSpace + 1) : param;

            fAst.paramTypes.push_back(pType);
            root->addChild(std::make_shared<ASTNode>(ASTNodeType::ParamDecl, pName, pType));
        }

        auto bodyNode = std::make_shared<ASTNode>(ASTNodeType::CompoundStmt, "Block");

        std::stringstream bss(bodyStr);
        std::string line;
        while (std::getline(bss, line)) {
            line.erase(0, line.find_first_not_of(" \t"));
            if (line.empty() || line.rfind("//", 0) == 0) continue;

            if (line.find("return") != std::string::npos) {
                auto retNode = std::make_shared<ASTNode>(ASTNodeType::ReturnStmt, "Return");
                if (line.find('*') != std::string::npos) {
                    auto binOp = std::make_shared<ASTNode>(ASTNodeType::BinaryOperator, "", "", "*");
                    binOp->addChild(std::make_shared<ASTNode>(ASTNodeType::VarRef, "var_left"));
                    binOp->addChild(std::make_shared<ASTNode>(ASTNodeType::VarRef, "var_right"));
                    retNode->addChild(binOp);
                } else if (line.find('+') != std::string::npos) {
                    auto binOp = std::make_shared<ASTNode>(ASTNodeType::BinaryOperator, "", "", "+");
                    binOp->addChild(std::make_shared<ASTNode>(ASTNodeType::VarRef, "var_left"));
                    binOp->addChild(std::make_shared<ASTNode>(ASTNodeType::VarRef, "var_right"));
                    retNode->addChild(binOp);
                } else if (line.find("==") != std::string::npos) {
                    auto binOp = std::make_shared<ASTNode>(ASTNodeType::BinaryOperator, "", "", "==");
                    binOp->addChild(std::make_shared<ASTNode>(ASTNodeType::VarRef, "param_1"));
                    binOp->addChild(std::make_shared<ASTNode>(ASTNodeType::VarRef, "param_2"));
                    retNode->addChild(binOp);
                } else {
                    retNode->addChild(std::make_shared<ASTNode>(ASTNodeType::Literal, "", "", "expr"));
                }
                bodyNode->addChild(retNode);
            } else if (line.find("if") != std::string::npos) {
                auto ifNode = std::make_shared<ASTNode>(ASTNodeType::IfStmt, "If");
                ifNode->addChild(std::make_shared<ASTNode>(ASTNodeType::BinaryOperator, "", "", "cond"));
                bodyNode->addChild(ifNode);
            } else if (line.find('=') != std::string::npos) {
                const auto equalsPosition = line.find('=');
                const auto declaration = line.substr(0, equalsPosition);
                const auto nameEnd = declaration.find_last_not_of(" \t");
                const auto nameStart = declaration.find_last_of(" \t", nameEnd);
                const std::string variableName = declaration.substr(nameStart + 1, nameEnd - nameStart);
                const std::string variableType = declaration.substr(0, nameStart);
                auto declNode = std::make_shared<ASTNode>(
                    ASTNodeType::VarDecl, variableName, variableType);
                if (auto expression = parseExpression(line.substr(equalsPosition + 1))) {
                    declNode->addChild(expression);
                }
                bodyNode->addChild(declNode);
            }
        }

        root->addChild(bodyNode);
        fAst.rootNode = root;
        functions.push_back(fAst);
    }

    return functions;
}
