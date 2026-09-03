#include "incppbuild/cache/CacheManager.hpp"

#include <cassert>
#include <filesystem>

int main() {
    const std::filesystem::path cachePath = "cache_manager_test.json";
    std::filesystem::remove(cachePath);

    CacheManager writer(cachePath.string());
    CachedFunctionInfo entry;
    entry.sourceFile = "src/login.cpp";
    entry.functionName = "login";
    entry.canonicalAST = "FUNCTION(login)";
    entry.fingerprint = "function-hash";
    entry.interfaceFingerprint = "interface-hash";
    entry.implementationFingerprint = "implementation-hash";
    entry.translationUnitFingerprint = "tu-hash";
    entry.objectPath = "cache/login.o";
    entry.compilerInfo = "clang++";
    entry.dependencies = {"include/login.h", "include/user.h"};
    writer.updateEntry(entry);
    assert(writer.saveCache());

    CacheManager reader(cachePath.string());
    assert(reader.loadCache());
    assert(reader.hasValidEntry("src/login.cpp", "login", "function-hash"));
    const auto* loaded = reader.getEntry("src/login.cpp", "login");
    assert(loaded != nullptr);
    assert(loaded->interfaceFingerprint == "interface-hash");
    assert(loaded->implementationFingerprint == "implementation-hash");
    assert(loaded->translationUnitFingerprint == "tu-hash");
    assert(loaded->objectPath == "cache/login.o");
    assert(loaded->dependencies.size() == 2);

    std::filesystem::remove(cachePath);
}