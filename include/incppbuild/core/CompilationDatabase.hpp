#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace incppbuild {

struct CompileCommand {
    std::filesystem::path directory;
    std::filesystem::path file;
    std::vector<std::string> arguments;
};

// Minimal reader for the standard JSON compilation database emitted by CMake
// with CMAKE_EXPORT_COMPILE_COMMANDS=ON. It supports both the preferred
// "arguments" form and the legacy shell-command "command" form.
class CompilationDatabase {
public:
    [[nodiscard]] static std::optional<CompilationDatabase> load_from_file(
        const std::filesystem::path& path, std::string* error = nullptr);

    [[nodiscard]] const std::vector<CompileCommand>& commands() const noexcept;
    [[nodiscard]] std::optional<CompileCommand> command_for(
        const std::filesystem::path& source_file) const;

    // Returns only arguments relevant to Clang parsing: compiler executable,
    // input source, output path, and compile-only switches are removed.
    [[nodiscard]] static std::vector<std::string> clang_arguments_for(
        const CompileCommand& command);

private:
    std::vector<CompileCommand> commands_;
};

}  // namespace incppbuild
