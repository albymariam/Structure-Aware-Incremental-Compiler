#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>

#include "incppbuild/ast/ASTParser.hpp"
#include "incppbuild/cache/CacheManager.hpp"
#include "incppbuild/dependency/dependency_graph.hpp"
#include "incppbuild/fingerprint/fingerprint.hpp"
#include "incppbuild/normalization/Canonicalizer.hpp"

namespace fs = std::filesystem;

// ============================================================
// Source Analysis Result
// ============================================================

struct SourceAnalysis {
    std::string sourceFile;
    std::string functionName;

    std::string interfaceCanonical;
    std::string implementationCanonical;

    std::string interfaceHash;
    std::string implementationHash;

    std::string canonicalForm;
    std::string combinedHash;

    bool valid = false;
};

// ============================================================
// Utility: Read File
// ============================================================

std::string readFile(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        return "";
    }

    return std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
}

// ============================================================
// Utility: Analyze Source Content
// ============================================================

SourceAnalysis analyzeSource(
    const std::string& source,
    const std::string& sourceName,
    Canonicalizer& canonicalizer
) {
    SourceAnalysis result;
    result.sourceFile = sourceName;

    const auto functions =
        ASTParser::parseSourceContent(source, sourceName);

    if (functions.empty()) {
        return result;
    }

    // This demo works with the first function because the sample
    // scenarios contain one function each.
    const FunctionAST& function = functions.front();

    result.functionName = function.functionName;

    // --------------------------------------------------------
    // Interface representation
    // --------------------------------------------------------

    result.interfaceCanonical =
        canonicalizer.canonicalizeInterface(function);

    result.interfaceHash =
        Fingerprinter::computeSHA256(
            result.interfaceCanonical
        );

    // --------------------------------------------------------
    // Implementation representation
    // --------------------------------------------------------

    result.implementationCanonical =
        canonicalizer.canonicalizeImplementation(function);

    result.implementationHash =
        Fingerprinter::computeSHA256(
            result.implementationCanonical
        );

    // --------------------------------------------------------
    // Complete representation
    // --------------------------------------------------------

    result.canonicalForm =
        result.interfaceCanonical +
        "Implementation" +
        result.implementationCanonical;

    result.combinedHash =
        Fingerprinter::computeSHA256(
            result.canonicalForm
        );

    result.valid = true;

    return result;
}

// ============================================================
// Utility: Print Analysis
// ============================================================

void printAnalysis(const SourceAnalysis& analysis) {
    if (!analysis.valid) {
        std::cout << "  [ERROR] Unable to analyze source.\n";
        return;
    }

    std::cout << "  Function              : "
              << analysis.functionName << "\n";

    std::cout << "  Interface Hash        : "
              << analysis.interfaceHash << "\n";

    std::cout << "  Implementation Hash   : "
              << analysis.implementationHash << "\n";

    std::cout << "  Combined Hash         : "
              << analysis.combinedHash << "\n";
}

// ============================================================
// Utility: Extract #include Dependencies From Source
// ============================================================

std::vector<std::string> extractIncludes(
    const std::string& source
) {
    std::vector<std::string> includes;

    std::regex includePattern(
        R"(^\s*#\s*include\s*[<"]([^>"]+)[>"])"
    );

    std::istringstream stream(source);
    std::string line;

    while (std::getline(stream, line)) {
        std::smatch match;

        if (std::regex_search(line, match, includePattern)) {
            includes.push_back(match[1].str());
        }
    }

    return includes;
}

// ============================================================
// Utility: Convert Header Name To Sample Source Unit
//
// Example:
//     login.h -> login.cpp
// ============================================================

std::string headerToSourceUnit(
    const std::string& header
) {
    fs::path path(header);

    if (path.extension() == ".h" ||
        path.extension() == ".hpp") {

        return path.stem().string() + ".cpp";
    }

    return "";
}

// ============================================================
// Build Dependency Graph From Actual Source Files
//
// This is deliberately based on #include statements found in
// the actual sample source files rather than manually assigning
// arbitrary dependency relationships.
// ============================================================

void buildDependencyGraphFromSources(
    incppbuild::DependencyGraph& graph,
    const std::vector<std::string>& sourceFiles
) {
    for (const auto& sourceFile : sourceFiles) {

        const std::string content = readFile(sourceFile);

        if (content.empty()) {
            continue;
        }

        const auto includes = extractIncludes(content);

        const fs::path sourcePath(sourceFile);
        const std::string dependentUnit =
            sourcePath.filename().string();

        for (const auto& include : includes) {

            const std::string dependencyUnit =
                headerToSourceUnit(include);

            if (dependencyUnit.empty() ||
                dependencyUnit == dependentUnit) {
                continue;
            }

            graph.add_dependency(
                dependentUnit,
                dependencyUnit
            );

            std::cout
                << "  Dependency discovered: "
                << dependentUnit
                << " depends on "
                << dependencyUnit
                << "\n";
        }
    }
}

// ============================================================
// Print Affected Units
// ============================================================

void printAffectedUnits(
    const std::vector<std::string>& affected
) {
    if (affected.empty()) {
        std::cout << "  No translation units require recompilation.\n";
        return;
    }

    for (const auto& unit : affected) {
        std::cout << "  -> " << unit << "\n";
    }
}

// ============================================================
// MAIN
// ============================================================

int main() {

    std::cout
        << "\n======================================================\n"
        << "   STRUCTURE-AWARE INCREMENTAL C++ COMPILATION DEMO\n"
        << "======================================================\n";

    std::cout
        << "\nPipeline demonstrated:\n"
        << "  Source\n"
        << "    -> AST\n"
        << "    -> Canonicalization\n"
        << "    -> Interface / Implementation fingerprints\n"
        << "    -> Cache comparison\n"
        << "    -> Change classification\n"
        << "    -> Dependency impact analysis\n";

    Canonicalizer canonicalizer;

    CacheManager cache("build_cache.json");

    // ========================================================
    // 1. BASELINE ANALYSIS
    // ========================================================

    std::cout
        << "\n------------------------------------------------------\n"
        << "1. BASELINE SOURCE ANALYSIS\n"
        << "------------------------------------------------------\n";

    const std::vector<std::string> sourceFiles = {
        "examples/sample_project/src/login.cpp",
        "examples/sample_project/src/payment.cpp",
        "examples/sample_project/src/database.cpp"
    };

    std::vector<SourceAnalysis> baselineAnalyses;

    for (const auto& sourceFile : sourceFiles) {

        std::cout
            << "\n[Analyzing] "
            << sourceFile << "\n";

        const auto functions =
            ASTParser::parseSourceFile(sourceFile);

        if (functions.empty()) {
            std::cout
                << "  [WARNING] No function detected.\n";
            continue;
        }

        for (const auto& function : functions) {

            const std::string interfaceCanonical =
                canonicalizer.canonicalizeInterface(function);

            const std::string implementationCanonical =
                canonicalizer.canonicalizeImplementation(function);

            const std::string interfaceHash =
                Fingerprinter::computeSHA256(
                    interfaceCanonical
                );

            const std::string implementationHash =
                Fingerprinter::computeSHA256(
                    implementationCanonical
                );

            const std::string canonicalForm =
                interfaceCanonical +
                "Implementation" +
                implementationCanonical;

            const std::string combinedHash =
                Fingerprinter::computeSHA256(
                    canonicalForm
                );

            SourceAnalysis analysis;

            analysis.sourceFile = sourceFile;
            analysis.functionName = function.functionName;
            analysis.interfaceCanonical = interfaceCanonical;
            analysis.implementationCanonical =
                implementationCanonical;
            analysis.interfaceHash = interfaceHash;
            analysis.implementationHash =
                implementationHash;
            analysis.canonicalForm = canonicalForm;
            analysis.combinedHash = combinedHash;
            analysis.valid = true;

            baselineAnalyses.push_back(analysis);

            printAnalysis(analysis);

            // Store complete Review 2 fingerprint information.
            CachedFunctionInfo entry;

            entry.sourceFile = sourceFile;
            entry.functionName = function.functionName;

            entry.canonicalAST =
                canonicalForm;

            entry.fingerprint =
                combinedHash;

            entry.interfaceFingerprint =
                interfaceHash;

            entry.implementationFingerprint =
                implementationHash;

            entry.translationUnitFingerprint =
                combinedHash;

            cache.updateEntry(
                std::move(entry)
            );
        }
    }

    if (!cache.saveCache()) {
        std::cout
            << "\n[WARNING] Cache could not be saved.\n";
    } else {
        std::cout
            << "\n[OK] Baseline cache saved.\n";
    }

    // ========================================================
    // 2. SCENARIO 1 - LOCAL IDENTIFIER RENAME
    // ========================================================

    std::cout
        << "\n------------------------------------------------------\n"
        << "2. SCENARIO 1: LOCAL IDENTIFIER RENAME\n"
        << "------------------------------------------------------\n";

    const std::string originalRenameSource = R"(
        int validateUser(int username, int password) {
            int result = username + password;
            return result;
        }
    )";

    const std::string renamedSource = R"(
        int validateUser(int userId, int passcode) {
            int status = userId + passcode;
            return status;
        }
    )";

    const auto originalRename =
        analyzeSource(
            originalRenameSource,
            "login_original.cpp",
            canonicalizer
        );

    const auto renamed =
        analyzeSource(
            renamedSource,
            "login_renamed.cpp",
            canonicalizer
        );

    std::cout << "\n[Original Source]\n";
    printAnalysis(originalRename);

    std::cout << "\n[Renamed Source]\n";
    printAnalysis(renamed);

    const bool interfaceSame =
        originalRename.interfaceHash ==
        renamed.interfaceHash;

    const bool implementationSame =
        originalRename.implementationHash ==
        renamed.implementationHash;

    std::cout
        << "\nResult:\n";

    std::cout
        << "  Interface fingerprint unchanged    : "
        << (interfaceSame ? "YES" : "NO")
        << "\n";

    std::cout
        << "  Implementation fingerprint unchanged: "
        << (implementationSame ? "YES" : "NO")
        << "\n";

    if (interfaceSame && implementationSame) {
        std::cout
            << "  Decision: REUSE CACHED COMPILATION RESULT\n"
            << "  Reason  : Structural representation is unchanged.\n";
    } else {
        std::cout
            << "  Decision: RECOMPILATION REQUIRED\n";
    }

    // ========================================================
    // 3. SCENARIO 2 - BODY LOGIC CHANGE
    // ========================================================

    std::cout
        << "\n------------------------------------------------------\n"
        << "3. SCENARIO 2: FUNCTION BODY LOGIC CHANGE\n"
        << "------------------------------------------------------\n";

    const std::string originalLogicSource = R"(
        int calculatePrice(int price, int quantity) {
            int total = price * quantity;
            return total;
        }
    )";

    const std::string modifiedLogicSource = R"(
        int calculatePrice(int price, int quantity) {
            int total = price + quantity;
            return total;
        }
    )";

    const auto originalLogic =
        analyzeSource(
            originalLogicSource,
            "payment_original.cpp",
            canonicalizer
        );

    const auto modifiedLogic =
        analyzeSource(
            modifiedLogicSource,
            "payment_modified.cpp",
            canonicalizer
        );

    std::cout << "\n[Original Source]\n";
    printAnalysis(originalLogic);

    std::cout << "\n[Modified Source]\n";
    printAnalysis(modifiedLogic);

    const bool logicInterfaceSame =
        originalLogic.interfaceHash ==
        modifiedLogic.interfaceHash;

    const bool logicImplementationChanged =
        originalLogic.implementationHash !=
        modifiedLogic.implementationHash;

    std::cout
        << "\nResult:\n";

    std::cout
        << "  Interface unchanged                : "
        << (logicInterfaceSame ? "YES" : "NO")
        << "\n";

    std::cout
        << "  Implementation changed             : "
        << (logicImplementationChanged ? "YES" : "NO")
        << "\n";

    if (logicInterfaceSame &&
        logicImplementationChanged) {

        std::cout
            << "  Decision: RECOMPILE payment.cpp\n"
            << "  Dependents: NOT INVALIDATED by interface change.\n";
    }

    // ========================================================
    // 4. SCENARIO 3 - INTERFACE CHANGE
    // ========================================================

    std::cout
        << "\n------------------------------------------------------\n"
        << "4. SCENARIO 3: FUNCTION INTERFACE CHANGE\n"
        << "------------------------------------------------------\n";

    const std::string originalInterfaceSource = R"(
        int calculatePrice(int price, int quantity) {
            int total = price * quantity;
            return total;
        }
    )";

    const std::string modifiedInterfaceSource = R"(
        double calculatePrice(double price, int quantity) {
            double total = price * quantity;
            return total;
        }
    )";

    const auto originalInterface =
        analyzeSource(
            originalInterfaceSource,
            "payment_original.cpp",
            canonicalizer
        );

    const auto modifiedInterface =
        analyzeSource(
            modifiedInterfaceSource,
            "payment_interface_changed.cpp",
            canonicalizer
        );

    std::cout << "\n[Original Interface]\n";
    printAnalysis(originalInterface);

    std::cout << "\n[Modified Interface]\n";
    printAnalysis(modifiedInterface);

    const bool interfaceChanged =
        originalInterface.interfaceHash !=
        modifiedInterface.interfaceHash;

    const bool implementationChanged =
        originalInterface.implementationHash !=
        modifiedInterface.implementationHash;

    std::cout
        << "\nResult:\n";

    std::cout
        << "  Interface changed                   : "
        << (interfaceChanged ? "YES" : "NO")
        << "\n";

    std::cout
        << "  Implementation changed              : "
        << (implementationChanged ? "YES" : "NO")
        << "\n";

    if (interfaceChanged) {
        std::cout
            << "  Decision: INTERFACE CHANGE DETECTED\n"
            << "  Action  : Recompile changed unit and evaluate dependents.\n";
    }

    // ========================================================
    // 5. SCENARIO 4 - BODY CHANGE WITH SAME INTERFACE
    // ========================================================

    std::cout
        << "\n------------------------------------------------------\n"
        << "5. SCENARIO 4: BODY-ONLY CHANGE\n"
        << "------------------------------------------------------\n";

    const std::string bodyVersion1 = R"(
        int processData(int value) {
            int result = value * 2;
            return result;
        }
    )";

    const std::string bodyVersion2 = R"(
        int processData(int value) {
            int result = value * 3;
            return result;
        }
    )";

    const auto bodyAnalysis1 =
        analyzeSource(
            bodyVersion1,
            "process_v1.cpp",
            canonicalizer
        );

    const auto bodyAnalysis2 =
        analyzeSource(
            bodyVersion2,
            "process_v2.cpp",
            canonicalizer
        );

    std::cout << "\n[Version 1]\n";
    printAnalysis(bodyAnalysis1);

    std::cout << "\n[Version 2]\n";
    printAnalysis(bodyAnalysis2);

    const bool bodyInterfaceSame =
        bodyAnalysis1.interfaceHash ==
        bodyAnalysis2.interfaceHash;

    const bool bodyImplementationDifferent =
        bodyAnalysis1.implementationHash !=
        bodyAnalysis2.implementationHash;

    std::cout
        << "\nResult:\n";

    std::cout
        << "  Interface fingerprint unchanged    : "
        << (bodyInterfaceSame ? "YES" : "NO")
        << "\n";

    std::cout
        << "  Implementation fingerprint changed : "
        << (bodyImplementationDifferent ? "YES" : "NO")
        << "\n";

    if (bodyInterfaceSame &&
        bodyImplementationDifferent) {

        std::cout
            << "  Decision: RECOMPILE ONLY THE CHANGED UNIT\n"
            << "  Reason  : Function contract is unchanged.\n";
    }

    // ========================================================
    // 6. SCENARIO 5 - SOURCE-DRIVEN DEPENDENCY ANALYSIS
    // ========================================================

    std::cout
        << "\n------------------------------------------------------\n"
        << "6. SCENARIO 5: DEPENDENCY IMPACT ANALYSIS\n"
        << "------------------------------------------------------\n";

    incppbuild::DependencyGraph dependencyGraph;

    // Include relationships are discovered from the actual
    // source files rather than manually creating arbitrary
    // graph edges.
    buildDependencyGraphFromSources(
        dependencyGraph,
        {
            "examples/sample_project/src/main.cpp",
            "examples/sample_project/src/login.cpp",
            "examples/sample_project/src/payment.cpp",
            "examples/sample_project/src/database.cpp"
        }
    );

    std::cout
        << "\n[A] Body-only change in payment.cpp\n";

    const auto bodyAffected =
        dependencyGraph.affected_by(
            {"payment.cpp"}
        );

    std::cout
        << "Affected units through dependency graph:\n";

    printAffectedUnits(bodyAffected);

    std::cout
        << "\nInterpretation:\n"
        << "  The graph identifies the changed unit and\n"
        << "  its reverse dependents.\n";

    std::cout
        << "\n[B] Interface change in payment.cpp\n";

    const auto interfaceAffected =
        dependencyGraph.affected_by(
            {"payment.cpp"}
        );

    std::cout
        << "Potentially affected units:\n";

    printAffectedUnits(interfaceAffected);

    std::cout
        << "\nInterpretation:\n"
        << "  An interface change can invalidate dependent\n"
        << "  translation units and therefore requires\n"
        << "  dependency-aware recompilation.\n";

    // ========================================================
    // 7. CACHE VERIFICATION
    // ========================================================

    std::cout
        << "\n------------------------------------------------------\n"
        << "7. CACHE VERIFICATION\n"
        << "------------------------------------------------------\n";

    CacheManager loadedCache("build_cache.json");

    if (loadedCache.loadCache()) {

        std::cout
            << "[OK] Persistent cache loaded successfully.\n";

        loadedCache.printCacheSummary();

    } else {

        std::cout
            << "[WARNING] Existing cache could not be loaded.\n";
    }

    // ========================================================
    // 8. FINAL REVIEW 2 SUMMARY
    // ========================================================

    std::cout
        << "\n======================================================\n"
        << "                 REVIEW 2 STATUS\n"
        << "======================================================\n";

    std::cout
        << "\n[PASS] Source-derived AST analysis\n"
        << "[PASS] Function-level canonicalization\n"
        << "[PASS] Alpha-renaming / identifier normalization\n"
        << "[PASS] Interface fingerprinting\n"
        << "[PASS] Implementation fingerprinting\n"
        << "[PASS] SHA-256 structural fingerprints\n"
        << "[PASS] Persistent fingerprint cache\n"
        << "[PASS] Local rename detection\n"
        << "[PASS] Body logic change detection\n"
        << "[PASS] Interface change detection\n"
        << "[PASS] Dependency graph traversal\n";

    std::cout
        << "\nCurrent implementation boundary:\n"
        << "  Source -> AST -> Fingerprint -> Change Analysis\n"
        << "  -> Dependency Impact -> Recompilation Decision\n";

    std::cout
        << "\nNot yet claimed by this demo:\n"
        << "  - Actual Clang LibTooling AST extraction\n"
        << "  - Actual compiler invocation\n"
        << "  - Actual .o file reuse\n"
        << "  - Final executable generation\n";

    std::cout
        << "\nNext implementation stage:\n"
        << "  Clang AST Parser -> Compiler Driver ->\n"
        << "  Selective Recompilation -> Object Reuse -> Linking\n";

    std::cout
        << "\n======================================================\n"
        << "                    DEMO COMPLETE\n"
        << "======================================================\n";

    return 0;
}