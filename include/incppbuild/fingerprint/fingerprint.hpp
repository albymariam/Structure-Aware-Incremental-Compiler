#ifndef FINGERPRINT_HPP
#define FINGERPRINT_HPP

#include <string>
#include <vector>
#include <cstdint>

#include "incppbuild/ast/ASTNode.hpp"

struct FunctionFingerprint {
    std::string functionName;
    std::string interfaceCanonicalForm;
    std::string implementationCanonicalForm;
    std::string interfaceHash;
    std::string implementationHash;
    std::string canonicalForm;
    std::string hash;
};

struct TranslationUnitFingerprint {
    std::string sourceName;
    std::vector<FunctionFingerprint> functions;
    std::string interfaceHash;
    std::string implementationHash;
    std::string hash;
};

class Fingerprinter {
public:
    // Computes SHA-256 hash string for given input text (e.g., canonicalized AST)
    static std::string computeSHA256(const std::string& input);

    // Produces a stable fingerprint for a single normalized function.
    static FunctionFingerprint fingerprintFunction(const FunctionAST& function);

    // Fingerprints a translation unit from its functions. Function entries are
    // sorted before aggregation, so source-order-only changes do not alter the
    // translation-unit fingerprint.
    static TranslationUnitFingerprint fingerprintTranslationUnit(
        const std::string& sourceName, const std::vector<FunctionAST>& functions);
};

#endif // FINGERPRINT_HPP
