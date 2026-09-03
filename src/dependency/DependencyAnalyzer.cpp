#include "incppbuild/dependency/DependencyAnalyzer.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/FileEntry.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Version.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/Tooling.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

namespace incppbuild {
namespace {

class IncludeCollector final : public clang::PPCallbacks {
public:
    IncludeCollector(const clang::SourceManager& sourceManager, DependencyAnalysis& analysis)
        : sourceManager_(sourceManager), analysis_(analysis) {}

#if CLANG_VERSION_MAJOR >= 15
    void InclusionDirective(clang::SourceLocation hashLocation,
                            const clang::Token&,
                            llvm::StringRef fileName,
                            bool,
                            clang::CharSourceRange,
                            clang::OptionalFileEntryRef file,
                            llvm::StringRef,
                            llvm::StringRef,
                            const clang::Module*,
                            bool,
                            clang::SrcMgr::CharacteristicKind) override {
        record(hashLocation, fileName, file ? file->getName() : fileName);
    }
#else
    void InclusionDirective(clang::SourceLocation hashLocation,
                            const clang::Token&,
                            llvm::StringRef fileName,
                            bool,
                            clang::CharSourceRange,
                            const clang::FileEntry* file,
                            llvm::StringRef,
                            llvm::StringRef,
                            const clang::Module*,
                            clang::SrcMgr::CharacteristicKind) override {
        record(hashLocation, fileName, file ? file->getName() : fileName);
    }
#endif

private:
    void record(clang::SourceLocation hashLocation, llvm::StringRef requestedName,
                llvm::StringRef resolvedName) {
        const auto dependent = sourceManager_.getFilename(hashLocation);
        if (dependent.empty()) return;
        analysis_.edges.push_back({std::string(dependent),
                                   std::string(resolvedName.empty() ? requestedName : resolvedName)});
    }

    const clang::SourceManager& sourceManager_;
    DependencyAnalysis& analysis_;
};

class DependencyAction final : public clang::ASTFrontendAction {
public:
    explicit DependencyAction(DependencyAnalysis& analysis) : analysis_(analysis) {}

    bool BeginSourceFileAction(clang::CompilerInstance& compiler) override {
        compiler.getPreprocessor().addPPCallbacks(
            std::make_unique<IncludeCollector>(compiler.getSourceManager(), analysis_));
        return true;
    }

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance&, llvm::StringRef) override {
        return std::make_unique<clang::ASTConsumer>();
    }

private:
    DependencyAnalysis& analysis_;
};

std::filesystem::path command_source_path(const CompileCommand& command) {
    return command.file.is_absolute() ? command.file.lexically_normal()
                                      : (command.directory / command.file).lexically_normal();
}

}  // namespace

std::optional<DependencyAnalysis> DependencyAnalyzer::analyzeFile(const CompileCommand& command) {
    const auto sourcePath = command_source_path(command);
    std::ifstream input(sourcePath, std::ios::binary);
    if (!input) return std::nullopt;

    const std::string source((std::istreambuf_iterator<char>(input)), {});
    DependencyAnalysis analysis;
    analysis.translationUnit = sourcePath;
    const auto arguments = CompilationDatabase::clang_arguments_for(command);
    const bool succeeded = clang::tooling::runToolOnCodeWithArgs(
        std::make_unique<DependencyAction>(analysis), source, arguments, sourcePath.string(),
        "incppbuild");
    if (!succeeded) return std::nullopt;

    std::sort(analysis.edges.begin(), analysis.edges.end(),
        [](const DependencyEdge& left, const DependencyEdge& right) {
            if (left.dependent != right.dependent) return left.dependent < right.dependent;
            return left.dependency < right.dependency;
        });
    analysis.edges.erase(std::unique(analysis.edges.begin(), analysis.edges.end(),
        [](const DependencyEdge& left, const DependencyEdge& right) {
            return left.dependent == right.dependent && left.dependency == right.dependency;
        }), analysis.edges.end());
    return analysis;
}

std::vector<DependencyAnalysis> DependencyAnalyzer::analyze(const CompilationDatabase& database) {
    std::vector<DependencyAnalysis> results;
    for (const auto& command : database.commands()) {
        if (auto analysis = analyzeFile(command)) results.push_back(std::move(*analysis));
    }
    return results;
}

void DependencyAnalyzer::addToGraph(const DependencyAnalysis& analysis, DependencyGraph& graph) {
    for (const auto& edge : analysis.edges) {
        graph.add_dependency(edge.dependent.generic_string(), edge.dependency.generic_string());
    }
}

}  // namespace incppbuild
