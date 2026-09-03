#include "incppbuild/core/change.hpp"
#include "incppbuild/ast/ASTParser.hpp"
#include "incppbuild/fingerprint/fingerprint.hpp"
#include "incppbuild/build/IncrementalBuilder.hpp"

#include <iostream>
#include <string>

namespace {

void printUsage(std::ostream& output) {
    output << "Usage: incppbuild build [compile_commands.json]\n"
           << "       incppbuild analyze <old.cpp> <new.cpp>\n"
           << "       incppbuild --help\n";
}

int build(const std::string& databasePath) {
    incppbuild::IncrementalBuilder builder(databasePath);
    const auto summary = builder.build();
    if (!summary.succeeded) {
        std::cerr << "incppbuild: " << summary.error << "\n";
        return 1;
    }
    incppbuild::IncrementalBuilder::print_report(summary, std::cout);
    return 0;
}

int analyze(const std::string& oldPath, const std::string& newPath) {
    const auto oldUnit = Fingerprinter::fingerprintTranslationUnit(
        oldPath, ASTParser::parseSourceFile(oldPath));
    const auto newUnit = Fingerprinter::fingerprintTranslationUnit(
        newPath, ASTParser::parseSourceFile(newPath));
    const auto impact = incppbuild::analyze_function_impact(oldUnit, newUnit);

    std::cout << "Translation unit: " << newPath << "\n";
    bool requiresRebuild = false;
    for (const auto& function : impact) {
        std::cout << "  " << function.functionName << ": "
                  << incppbuild::to_string(function.level) << "\n";
        requiresRebuild = requiresRebuild ||
            incppbuild::requires_recompilation(function.level);
    }
    std::cout << "Affected: " << (requiresRebuild ? newPath : "none") << "\n";
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        printUsage(std::cout);
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "analyze") {
        return analyze(argv[2], argv[3]);
    }
    if ((argc == 2 || argc == 3) && std::string(argv[1]) == "build") {
        return build(argc == 3 ? argv[2] : "build/compile_commands.json");
    }

    printUsage(std::cerr);
    return 2;
}
