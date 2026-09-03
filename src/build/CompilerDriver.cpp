#include "incppbuild/build/CompilerDriver.hpp"

#include <cstdlib>
#include <sstream>

namespace incppbuild {
namespace {

std::string quote(const std::string& value) {
    std::string result = "\"";
    for (const char character : value) {
        if (character == '"') result += '\\';
        result += character;
    }
    return result + '"';
}

std::filesystem::path source_path(const CompileCommand& command) {
    if (command.file.is_absolute()) return command.file;
    return command.directory / command.file;
}

bool is_source_argument(const std::string& argument, const CompileCommand& command) {
    const auto source = source_path(command).lexically_normal().string();
    return argument == command.file.string() ||
           std::filesystem::path(argument).lexically_normal().string() == source;
}

}  // namespace

CompileResult CompilerDriver::compile(const CompileCommand& command,
                                      const std::filesystem::path& object_path) {
    std::error_code error;
    std::filesystem::create_directories(object_path.parent_path(), error);
    if (error) return {false, -1, ""};

    std::ostringstream invocation;
    for (std::size_t index = 0; index < command.arguments.size(); ++index) {
        const auto& argument = command.arguments[index];
        if (index == 0 || argument == "-c" || argument == "/c" ||
            is_source_argument(argument, command)) {
            continue;
        }
        if (argument == "-o" || argument == "/Fo") {
            ++index;
            continue;
        }
        if (argument.rfind("/Fo", 0) == 0) continue;
        invocation << quote(argument) << ' ';
    }
    invocation << "-c " << quote(source_path(command).string())
               << " -o " << quote(object_path.string());

    const auto command_line = invocation.str();
    const auto exit_code = std::system(command_line.c_str());
    return {exit_code == 0, exit_code, command_line};
}

}  // namespace incppbuild