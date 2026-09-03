#ifndef FINGERPRINT_HPP
#define FINGERPRINT_HPP

#include <string>
#include <vector>
#include <cstdint>

class Fingerprinter {
public:
    // Computes SHA-256 hash string for given input text (e.g., canonicalized AST)
    static std::string computeSHA256(const std::string& input);
};

#endif // FINGERPRINT_HPP
