#pragma once

#include "incppbuild/ast/ASTNode.hpp"

#include <string>
#include <vector>

namespace incppbuild {

// Clang-backed replacement for the regex prototype parser. The caller supplies
// the compile arguments recorded for this translation unit, normally from
// compile_commands.json.
class ClangASTParser {
public:
    [[nodiscard]] static std::vector<FunctionAST> parse_source_file(
        const std::string& file_path,
        const std::vector<std::string>& compile_arguments);
};

}  // namespace incppbuild
