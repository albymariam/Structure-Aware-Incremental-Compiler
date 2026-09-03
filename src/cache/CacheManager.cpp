#include "incppbuild/cache/CacheManager.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>

void CacheManager::updateEntry(const std::string& file, const std::string& funcName, const std::string& canonicalAST, const std::string& fingerprint) {
    std::string key = file + "::" + funcName;
    CachedFunctionInfo info;
    info.sourceFile = file;
    info.functionName = funcName;
    info.canonicalAST = canonicalAST;
    info.fingerprint = fingerprint;
    info.lastModifiedTime = "2026-08-27 23:10:00"; // Timestamp placeholder
    cacheEntries[key] = info;
}

const CachedFunctionInfo* CacheManager::getEntry(const std::string& file, const std::string& funcName) const {
    std::string key = file + "::" + funcName;
    auto it = cacheEntries.find(key);
    if (it != cacheEntries.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool CacheManager::saveCache() {
    std::ofstream outFile(cacheFilePath);
    if (!outFile.is_open()) return false;

    outFile << "{\n";
    outFile << "  \"project\": \"Structure-Aware C++ Build System\",\n";
    outFile << "  \"cacheEntries\": [\n";
    
    size_t count = 0;
    for (const auto& pair : cacheEntries) {
        const auto& entry = pair.second;
        outFile << "    {\n";
        outFile << "      \"key\": \"" << pair.first << "\",\n";
        outFile << "      \"sourceFile\": \"" << entry.sourceFile << "\",\n";
        outFile << "      \"functionName\": \"" << entry.functionName << "\",\n";
        outFile << "      \"fingerprint\": \"" << entry.fingerprint << "\",\n";
        outFile << "      \"canonicalAST\": \"" << entry.canonicalAST << "\"\n";
        outFile << "    }" << (count + 1 < cacheEntries.size() ? "," : "") << "\n";
        count++;
    }

    outFile << "  ]\n";
    outFile << "}\n";
    return true;
}

bool CacheManager::loadCache() {
    std::ifstream inFile(cacheFilePath);
    return inFile.is_open();
}

void CacheManager::printCacheSummary() const {
    std::cout << "\n======================================================\n";
    std::cout << "          BUILD CACHE FINGERPRINT DATABASE            \n";
    std::cout << "======================================================\n";
    for (const auto& pair : cacheEntries) {
        const auto& e = pair.second;
        std::cout << "[File] " << e.sourceFile << " | [Func] " << e.functionName << "\n";
        std::cout << "  ├─ Fingerprint SHA-256 : " << e.fingerprint << "\n";
        std::cout << "  └─ Canonical AST Tree  : " << e.canonicalAST << "\n\n";
    }
    std::cout << "======================================================\n";
}
