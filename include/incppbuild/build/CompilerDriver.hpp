#pragma once

#include "incppbuild/core/CompilationDatabase.hpp"

#include <filesystem>
#include <string>

namespace incppbuild {

struct CompileResult {
    bool succeeded = false;
    int exitCode = -1;
    std::string command;
};

class CompilerDriver {
public:
    [[nodiscard]] static CompileResult compile(
        const CompileCommand& command,
        const std::filesystem::path& object_path);
};

}  // namespace incppbuild