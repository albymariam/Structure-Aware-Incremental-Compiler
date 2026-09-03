#include "incppbuild/core/CompilationDatabase.hpp"

#include <cctype>
#include <fstream>
#include <iterator>
#include <string_view>

namespace incppbuild {
namespace {

class JsonReader {
public:
    explicit JsonReader(std::string_view input) : input_(input) {}

    bool consume(char expected) {
        skip_whitespace();
        if (position_ == input_.size() || input_[position_] != expected) return false;
        ++position_;
        return true;
    }

    bool read_string(std::string& result) {
        skip_whitespace();
        if (position_ == input_.size() || input_[position_++] != '\"') return false;
        result.clear();
        while (position_ < input_.size()) {
            const char character = input_[position_++];
            if (character == '\"') return true;
            if (character != '\\') {
                result += character;
                continue;
            }
            if (position_ == input_.size()) return false;
            switch (input_[position_++]) {
            case '\"': result += '\"'; break;
            case '\\': result += '\\'; break;
            case '/': result += '/'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            // Compilation database paths and arguments are conventionally UTF-8.
            // Preserve a Unicode escape if present rather than silently corrupting it.
            case 'u':
                if (position_ + 4 > input_.size()) return false;
                result += "\\u";
                result.append(input_.substr(position_, 4));
                position_ += 4;
                break;
            default: return false;
            }
        }
        return false;
    }

    bool skip_value() {
        skip_whitespace();
        if (position_ == input_.size()) return false;
        if (input_[position_] == '\"') {
            std::string unused;
            return read_string(unused);
        }
        if (input_[position_] == '{') return skip_compound('{', '}');
        if (input_[position_] == '[') return skip_compound('[', ']');
        while (position_ < input_.size() && input_[position_] != ',' &&
               input_[position_] != ']' && input_[position_] != '}') {
            ++position_;
        }
        return true;
    }

private:
    void skip_whitespace() {
        while (position_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[position_]))) {
            ++position_;
        }
    }

    bool skip_compound(char open, char close) {
        if (input_[position_++] != open) return false;
        int depth = 1;
        while (position_ < input_.size() && depth != 0) {
            if (input_[position_] == '\"') {
                std::string unused;
                if (!read_string(unused)) return false;
            } else if (input_[position_] == open) {
                ++depth;
                ++position_;
            } else if (input_[position_] == close) {
                --depth;
                ++position_;
            } else {
                ++position_;
            }
        }
        return depth == 0;
    }

    std::string_view input_;
    std::size_t position_ = 0;
};

bool read_string_array(JsonReader& reader, std::vector<std::string>& values) {
    if (!reader.consume('[')) return false;
    values.clear();
    std::string value;
    while (true) {
        if (reader.consume(']')) return true;
        if (!reader.read_string(value)) return false;
        values.push_back(value);
        if (reader.consume(']')) return true;
        if (!reader.consume(',')) return false;
    }
}

std::vector<std::string> split_command(std::string_view command) {
    std::vector<std::string> result;
    std::string current;
    char quote = '\0';
    bool escaped = false;
    for (char character : command) {
        if (escaped) {
            current += character;
            escaped = false;
        } else if (character == '\\' && quote != '\'') {
            escaped = true;
        } else if ((character == '\"' || character == '\'') && (quote == '\0' || quote == character)) {
            quote = quote == '\0' ? character : '\0';
        } else if (std::isspace(static_cast<unsigned char>(character)) && quote == '\0') {
            if (!current.empty()) {
                result.push_back(std::move(current));
                current.clear();
            }
        } else {
            current += character;
        }
    }
    if (!current.empty()) result.push_back(std::move(current));
    return result;
}

std::filesystem::path absolute_from(const std::filesystem::path& path,
                                    const std::filesystem::path& directory) {
    if (path.is_absolute()) return path.lexically_normal();
    const auto base = directory.is_absolute() ? directory : std::filesystem::absolute(directory);
    return (base / path).lexically_normal();
}

}  // namespace

std::optional<CompilationDatabase> CompilationDatabase::load_from_file(
    const std::filesystem::path& path, std::string* error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        if (error) *error = "Cannot open compilation database: " + path.string();
        return std::nullopt;
    }
    const std::string content((std::istreambuf_iterator<char>(input)), {});
    JsonReader reader(content);
    if (!reader.consume('[')) {
        if (error) *error = "Compilation database must be a JSON array";
        return std::nullopt;
    }

    CompilationDatabase database;
    while (!reader.consume(']')) {
        if (!reader.consume('{')) {
            if (error) *error = "Expected a compile-command object";
            return std::nullopt;
        }
        std::string directory;
        std::string file;
        std::string shell_command;
        std::vector<std::string> arguments;
        while (!reader.consume('}')) {
            std::string key;
            if (!reader.read_string(key) || !reader.consume(':')) {
                if (error) *error = "Malformed compile-command object";
                return std::nullopt;
            }
            if (key == "arguments") {
                if (!read_string_array(reader, arguments)) {
                    if (error) *error = "arguments must be an array of strings";
                    return std::nullopt;
                }
            } else if (key == "directory" || key == "file" || key == "command") {
                std::string value;
                if (!reader.read_string(value)) {
                    if (error) *error = key + " must be a string";
                    return std::nullopt;
                }
                if (key == "directory") directory = std::move(value);
                else if (key == "file") file = std::move(value);
                else shell_command = std::move(value);
            } else if (!reader.skip_value()) {
                if (error) *error = "Malformed value for " + key;
                return std::nullopt;
            }
            if (reader.consume('}')) break;
            if (!reader.consume(',')) {
                if (error) *error = "Expected ',' between compile-command fields";
                return std::nullopt;
            }
        }
        if (directory.empty() || file.empty()) {
            if (error) *error = "Each compile command needs directory and file";
            return std::nullopt;
        }
        if (arguments.empty()) arguments = split_command(shell_command);
        if (arguments.empty()) {
            if (error) *error = "Each compile command needs arguments or command";
            return std::nullopt;
        }
        database.commands_.push_back({directory, file, std::move(arguments)});
        if (reader.consume(']')) break;
        if (!reader.consume(',')) {
            if (error) *error = "Expected ',' between compile-command objects";
            return std::nullopt;
        }
    }
    return database;
}

const std::vector<CompileCommand>& CompilationDatabase::commands() const noexcept {
    return commands_;
}

std::optional<CompileCommand> CompilationDatabase::command_for(
    const std::filesystem::path& source_file) const {
    const auto requested = std::filesystem::absolute(source_file).lexically_normal();
    for (const auto& command : commands_) {
        const auto candidate = std::filesystem::absolute(
            absolute_from(command.file, command.directory)).lexically_normal();
        if (candidate == requested) return command;
    }
    return std::nullopt;
}

std::vector<std::string> CompilationDatabase::clang_arguments_for(const CompileCommand& command) {
    std::vector<std::string> result;
    const auto source = absolute_from(command.file, command.directory);
    for (std::size_t index = 0; index < command.arguments.size(); ++index) {
        const auto& argument = command.arguments[index];
        if (index == 0 || argument == "-c" || argument == "/c") continue;
        if (argument == "-o" || argument == "/Fo") {
            ++index;
            continue;
        }
        if (argument.rfind("/Fo", 0) == 0) continue;

        const auto normalize_path = [&command](const std::string& path) {
            return absolute_from(path, command.directory).string();
        };
        if (argument == "-I" || argument == "/I" || argument == "-isystem" ||
            argument == "-iquote" || argument == "-include") {
            if (index + 1 < command.arguments.size()) {
                result.push_back(argument);
                result.push_back(normalize_path(command.arguments[++index]));
            }
            continue;
        }
        if (argument.rfind("-I", 0) == 0 && argument.size() > 2) {
            result.push_back("-I" + normalize_path(argument.substr(2)));
            continue;
        }
        if (argument.rfind("/I", 0) == 0 && argument.size() > 2) {
            result.push_back("/I" + normalize_path(argument.substr(2)));
            continue;
        }
        const auto candidate = absolute_from(argument, command.directory);
        if (candidate == source || argument == command.file.string()) continue;
        result.push_back(argument);
    }
    return result;
}

}  // namespace incppbuild
