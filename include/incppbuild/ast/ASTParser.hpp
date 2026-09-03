#ifndef AST_PARSER_HPP
#define AST_PARSER_HPP

#include "incppbuild/ast/ASTNode.hpp"
#include <string>
#include <vector>

class ASTParser {
public:
    // Parses C++ source code and extracts list of functions with their AST representations
    static std::vector<FunctionAST> parseSourceFile(const std::string& filepath);
    static std::vector<FunctionAST> parseSourceContent(const std::string& content, const std::string& filename);
};

#endif // AST_PARSER_HPP
