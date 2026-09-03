#ifndef CANONICALIZER_HPP
#define CANONICALIZER_HPP

#include "incppbuild/ast/ASTNode.hpp"
#include <string>
#include <unordered_map>

class Canonicalizer {
private:
    std::unordered_map<std::string, std::string> symbolMap;
    int varCounter;

    std::string getCanonicalName(const std::string& originalName);
    std::string canonicalizeNode(const std::shared_ptr<ASTNode>& node);

public:
    Canonicalizer() : varCounter(0) {}

    // Resets symbol mapping for a new function scope
    void reset();

    // Converts a FunctionAST into a canonicalized S-expression string
    std::string canonicalize(const FunctionAST& funcAST);
};

#endif // CANONICALIZER_HPP
