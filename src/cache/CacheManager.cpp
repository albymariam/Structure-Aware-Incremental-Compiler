#include "incppbuild/cache/CacheManager.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <sstream>

void CacheManager::updateEntry(const std::string& file, const std::string& funcName, const std::string& canonicalAST, const std::string& fingerprint) {
    CachedFunctionInfo info;
    info.sourceFile = file;
    info.functionName = funcName;
    info.canonicalAST = canonicalAST;
    info.fingerprint = fingerprint;
    info.interfaceFingerprint = fingerprint;
    info.implementationFingerprint = fingerprint;
    updateEntry(std::move(info));
}

void CacheManager::updateEntry(CachedFunctionInfo entry) {
    cacheEntries[entry.sourceFile + "::" + entry.functionName] = std::move(entry);
}

const CachedFunctionInfo* CacheManager::getEntry(const std::string& file, const std::string& funcName) const {
    std::string key = file + "::" + funcName;
    auto it = cacheEntries.find(key);
    if (it != cacheEntries.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool CacheManager::hasValidEntry(const std::string& file, const std::string& funcName,
                                 const std::string& expectedFingerprint) const {
    const auto* entry = getEntry(file, funcName);
    if (entry == nullptr || entry->fingerprint.empty()) return false;
    return expectedFingerprint.empty() || entry->fingerprint == expectedFingerprint;
}

const std::unordered_map<std::string, CachedFunctionInfo>& CacheManager::entries() const noexcept {
    return cacheEntries;
}

namespace {

std::string escape_json(const std::string& value) {
    std::string escaped;
    for (const char character : value) {
        if (character == '\\' || character == '"') escaped += '\\';
        if (character == '\n') escaped += "\\n";
        else escaped += character;
    }
    return escaped;
}

std::string unescape_json(std::string value) {
    std::string result;
    bool escaped = false;
    for (const char character : value) {
        if (escaped) {
            result += character == 'n' ? '\n' : character;
            escaped = false;
        } else if (character == '\\') {
            escaped = true;
        } else {
            result += character;
        }
    }
    return result;
}

std::string field(const std::string& object, const std::string& name) {
    const std::regex pattern("\\\"" + name + "\\\"\\s*:\\s*\\\"((?:\\\\.|[^\\\"])*)\\\"");
    std::smatch match;
    return std::regex_search(object, match, pattern) ? unescape_json(match[1].str()) : "";
}

std::vector<std::string> dependency_field(const std::string& object) {
    std::vector<std::string> result;
    const auto name = object.find("\"dependencies\"");
    if (name == std::string::npos) return result;
    const auto open = object.find('[', name);
    const auto close = object.find(']', open);
    if (open == std::string::npos || close == std::string::npos) return result;

    bool insideString = false;
    bool escaped = false;
    std::string value;
    for (std::size_t index = open + 1; index < close; ++index) {
        const char character = object[index];
        if (escaped) {
            value += character == 'n' ? '\n' : character;
            escaped = false;
        } else if (character == '\\' && insideString) {
            escaped = true;
        } else if (character == '"') {
            if (insideString) result.push_back(value);
            value.clear();
            insideString = !insideString;
        } else if (insideString) {
            value += character;
        }
    }
    return result;
}

}  // namespace

bool CacheManager::saveCache() {
    std::ofstream outFile(cacheFilePath);
    if (!outFile.is_open()) return false;

    outFile << "{\n";
    outFile << "  \"project\": \"Structure-Aware C++ Build System\",\n";
    outFile << "  \"cacheEntries\": [\n";
    
    std::vector<std::string> keys;
    keys.reserve(cacheEntries.size());
    for (const auto& pair : cacheEntries) keys.push_back(pair.first);
    std::sort(keys.begin(), keys.end());
    for (size_t index = 0; index < keys.size(); ++index) {
        const auto& key = keys[index];
        const auto& entry = cacheEntries.at(key);
        outFile << "    {\n";
        outFile << "      \"sourceFile\": \"" << escape_json(entry.sourceFile) << "\",\n";
        outFile << "      \"functionName\": \"" << escape_json(entry.functionName) << "\",\n";
        outFile << "      \"canonicalAST\": \"" << escape_json(entry.canonicalAST) << "\",\n";
        outFile << "      \"fingerprint\": \"" << escape_json(entry.fingerprint) << "\",\n";
        outFile << "      \"interfaceFingerprint\": \"" << escape_json(entry.interfaceFingerprint) << "\",\n";
        outFile << "      \"implementationFingerprint\": \"" << escape_json(entry.implementationFingerprint) << "\",\n";
        outFile << "      \"translationUnitFingerprint\": \"" << escape_json(entry.translationUnitFingerprint) << "\",\n";
        outFile << "      \"objectPath\": \"" << escape_json(entry.objectPath) << "\",\n";
        outFile << "      \"compilerInfo\": \"" << escape_json(entry.compilerInfo) << "\",\n";
        outFile << "      \"dependencies\": [";
        for (size_t dependency = 0; dependency < entry.dependencies.size(); ++dependency) {
            if (dependency != 0) outFile << ", ";
            outFile << "\"" << escape_json(entry.dependencies[dependency]) << "\"";
        }
        outFile << "]\n    }" << (index + 1 < keys.size() ? "," : "") << "\n";
    }

    outFile << "  ]\n";
    outFile << "}\n";
    return true;
}

bool CacheManager::loadCache() {
    std::ifstream inFile(cacheFilePath);
    if (!inFile.is_open()) return false;
    const std::string content((std::istreambuf_iterator<char>(inFile)), {});
    const std::regex object_pattern("\\{\\s*\\\"sourceFile\\\"[\\s\\S]*?\\n    \\}", std::regex::ECMAScript);
    cacheEntries.clear();
    for (std::sregex_iterator it(content.begin(), content.end(), object_pattern), end; it != end; ++it) {
        CachedFunctionInfo entry;
        const auto object = it->str();
        entry.sourceFile = field(object, "sourceFile");
        entry.functionName = field(object, "functionName");
        if (entry.sourceFile.empty() || entry.functionName.empty()) continue;
        entry.canonicalAST = field(object, "canonicalAST");
        entry.fingerprint = field(object, "fingerprint");
        entry.interfaceFingerprint = field(object, "interfaceFingerprint");
        entry.implementationFingerprint = field(object, "implementationFingerprint");
        entry.translationUnitFingerprint = field(object, "translationUnitFingerprint");
        entry.objectPath = field(object, "objectPath");
        entry.compilerInfo = field(object, "compilerInfo");
        entry.dependencies = dependency_field(object);
        updateEntry(std::move(entry));
    }
    return true;
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
