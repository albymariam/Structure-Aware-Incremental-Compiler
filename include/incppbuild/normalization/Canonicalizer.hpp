#ifndef CANONICALIZER_HPP
#define CANONICALIZER_HPP

#include "incppbuild/ast/ASTNode.hpp"
#include <string>
#include <unordered_map>

class Canonicalizer {
private:
    std::unordered_map<std::string, std::string> symbolMap;
    int varCounter;

    std::string declareCanonicalName(const std::string& originalName);
    std::string referenceCanonicalName(const std::string& originalName) const;
    static std::string canonicalizeType(const std::string& typeName);
    static std::string canonicalizeLiteral(const std::string& literal);
    std::string canonicalizeNode(const std::shared_ptr<ASTNode>& node);

public:
    Canonicalizer() : varCounter(0) {}

    // Resets symbol mapping for a new function scope
    void reset();

    // Converts a FunctionAST into a canonicalized S-expression string
    std::string canonicalize(const FunctionAST& funcAST);

    // The interface representation contains only externally relevant function
    // properties. The implementation representation contains normalized body
    // structure and intentionally excludes parameter/local spelling.
    std::string canonicalizeInterface(const FunctionAST& funcAST);
    std::string canonicalizeImplementation(const FunctionAST& funcAST);
};

#endif // CANONICALIZER_HPP
