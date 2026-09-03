#include "incppbuild/core/CompilationDatabase.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    const auto root = std::filesystem::temp_directory_path() / "incppbuild_compdb_test";
    std::filesystem::create_directories(root / "src");
    const auto database_path = root / "compile_commands.json";

    {
        std::ofstream output(database_path);
        output << R"([
          {
            "directory": ")" << root.generic_string() << R"(",
            "file": "src/widget.cpp",
            "arguments": ["clang++", "-std=c++20", "-Iinclude", "-DDEBUG", "-c", "src/widget.cpp", "-o", "widget.o"]
          }
        ])";
    }

    std::string error;
    const auto database = incppbuild::CompilationDatabase::load_from_file(database_path, &error);
    assert(database.has_value());
    assert(error.empty());
    assert(database->commands().size() == 1);

    const auto command = database->command_for(root / "src" / "widget.cpp");
    assert(command.has_value());
    const auto arguments = incppbuild::CompilationDatabase::clang_arguments_for(*command);
    assert(arguments.size() == 3);
    assert(arguments[0] == "-std=c++20");
    assert(arguments[1] == "-I" + (root / "include").string());
    assert(arguments[2] == "-DDEBUG");

    std::filesystem::remove_all(root);
}
