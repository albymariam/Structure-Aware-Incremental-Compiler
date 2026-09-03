#ifndef CACHE_MANAGER_HPP
#define CACHE_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <vector>

struct CachedFunctionInfo {
    std::string functionName;
    std::string sourceFile;
    std::string canonicalAST;
    std::string fingerprint;
    std::string lastModifiedTime;
};

class CacheManager {
private:
    std::string cacheFilePath;
    std::unordered_map<std::string, CachedFunctionInfo> cacheEntries; // Key: sourceFile::functionName

public:
    explicit CacheManager(const std::string& dbPath = "build_cache.json") : cacheFilePath(dbPath) {}

    bool loadCache();
    bool saveCache();

    void updateEntry(const std::string& file, const std::string& funcName, const std::string& canonicalAST, const std::string& fingerprint);
    const CachedFunctionInfo* getEntry(const std::string& file, const std::string& funcName) const;

    void printCacheSummary() const;
};

#endif // CACHE_MANAGER_HPP
