#include "incppbuild/build/IncrementalBuilder.hpp"

#include "incppbuild/ast/ASTParser.hpp"
#include "incppbuild/build/CompilerDriver.hpp"
#include "incppbuild/cache/CacheManager.hpp"
#include "incppbuild/core/CompilationDatabase.hpp"
#include "incppbuild/fingerprint/fingerprint.hpp"

#include <algorithm>
#include <map>
#include <ostream>

namespace incppbuild {
namespace {

FunctionFingerprint cached_fingerprint(const CachedFunctionInfo& entry) {
    FunctionFingerprint result;
    result.functionName = entry.functionName;
    result.interfaceHash = entry.interfaceFingerprint;
    result.implementationHash = entry.implementationFingerprint;
    result.hash = entry.fingerprint;
    return result;
}

TranslationUnitFingerprint previous_unit(const CacheManager& cache,
                                          const std::string& source) {
    TranslationUnitFingerprint result;
    result.sourceName = source;
    for (const auto& [key, entry] : cache.entries()) {
        if (entry.sourceFile != source) continue;
        result.functions.push_back(cached_fingerprint(entry));
    }
    return result;
}

std::filesystem::path object_path(const std::filesystem::path& directory,
                                  const std::filesystem::path& source) {
    auto object = directory / source.filename();
    object.replace_extension(".o");
    return object;
}

}  // namespace

IncrementalBuilder::IncrementalBuilder(std::filesystem::path compilation_database,
                                       std::filesystem::path cache_file,
                                       std::filesystem::path object_directory)
    : compilationDatabasePath_(std::move(compilation_database)),
      cacheFilePath_(std::move(cache_file)),
      objectDirectory_(std::move(object_directory)) {}

BuildSummary IncrementalBuilder::build() {
    BuildSummary summary;
    std::string error;
    const auto database = CompilationDatabase::load_from_file(compilationDatabasePath_, &error);
    if (!database) {
        summary.error = error;
        return summary;
    }

    CacheManager cache(cacheFilePath_.string());
    cache.loadCache();
    for (const auto& command : database->commands()) {
        const auto source = command.file.is_absolute() ? command.file
                                                        : command.directory / command.file;
        const auto parsedFunctions = ASTParser::parseSourceFile(source.string());
        const auto current = Fingerprinter::fingerprintTranslationUnit(
            source.string(), parsedFunctions);
        const auto old = previous_unit(cache, source.string());
        const auto changes = analyze_function_changes(old, current);
        summary.changes.insert(summary.changes.end(), changes.begin(), changes.end());

        const auto object = object_path(objectDirectory_, source);
        bool rebuild = changes.empty() && !std::filesystem::exists(object);
        for (const auto& change : changes) {
            rebuild = rebuild || requires_recompilation(change.level);
        }
        if (rebuild) {
            const auto result = CompilerDriver::compile(command, object);
            if (!result.succeeded) {
                summary.error = "Compilation failed for " + source.string();
                return summary;
            }
            summary.rebuilt.push_back(source);
        } else {
            summary.reused.push_back(source);
        }

        BuildReportEntry report;
        report.source = source;
        report.recompiled = rebuild;
        for (const auto& change : changes) {
            report.change = std::max(report.change, change.level);
            report.interfaceChanged = report.interfaceChanged ||
                change.level == ChangeLevel::interface_change;
            report.implementationChanged = report.implementationChanged ||
                change.level == ChangeLevel::body_logic;
        }
        summary.report.push_back(report);

        for (const auto& function : parsedFunctions) {
            const auto fingerprint = Fingerprinter::fingerprintFunction(function);
            CachedFunctionInfo entry;
            entry.sourceFile = source.string();
            entry.functionName = function.functionName;
            entry.canonicalAST = fingerprint.canonicalForm;
            entry.fingerprint = fingerprint.hash;
            entry.interfaceFingerprint = fingerprint.interfaceHash;
            entry.implementationFingerprint = fingerprint.implementationHash;
            entry.translationUnitFingerprint = current.hash;
            entry.objectPath = object.string();
            entry.compilerInfo = command.arguments.empty() ? "" : command.arguments.front();
            cache.updateEntry(std::move(entry));
        }
    }
    summary.succeeded = cache.saveCache();
    if (!summary.succeeded) summary.error = "Unable to save cache: " + cacheFilePath_.string();
    return summary;
}

void IncrementalBuilder::print_report(const BuildSummary& summary, std::ostream& output) {
    output << "========== Incremental Build Report ==========\n\n";
    for (const auto& entry : summary.report) {
        output << entry.source.string() << "\n"
               << "    Change        : " << to_string(entry.change) << "\n";
        if (entry.change != ChangeLevel::no_change) {
            output << "    Interface     : " << (entry.interfaceChanged ? "CHANGED" : "UNCHANGED") << "\n"
                   << "    Implementation: " << (entry.implementationChanged ? "CHANGED" : "UNCHANGED") << "\n";
        }
        output << "    Action        : " << (entry.recompiled ? "RECOMPILE" : "REUSE") << "\n\n";
    }
    output << "Affected files : " << summary.rebuilt.size() << "\n"
           << "Recompiled     : " << summary.rebuilt.size() << "\n"
           << "Reused         : " << summary.reused.size() << "\n\n"
           << "===============================================\n";
}

}  // namespace incppbuild