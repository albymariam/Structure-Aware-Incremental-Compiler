#pragma once

#include "incppbuild/core/change.hpp"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace incppbuild {

struct BuildReportEntry {
    std::filesystem::path source;
    ChangeLevel change = ChangeLevel::no_change;
    bool interfaceChanged = false;
    bool implementationChanged = false;
    bool recompiled = false;
};

struct BuildSummary {
    bool succeeded = false;
    std::string error;
    std::vector<std::filesystem::path> rebuilt;
    std::vector<std::filesystem::path> reused;
    std::vector<FunctionChange> changes;
    std::vector<BuildReportEntry> report;
};

class IncrementalBuilder {
public:
    IncrementalBuilder(std::filesystem::path compilation_database,
                       std::filesystem::path cache_file = "build_cache.json",
                       std::filesystem::path object_directory = "cache");

    [[nodiscard]] BuildSummary build();

    static void print_report(const BuildSummary& summary, std::ostream& output);

private:
    std::filesystem::path compilationDatabasePath_;
    std::filesystem::path cacheFilePath_;
    std::filesystem::path objectDirectory_;
};

}  // namespace incppbuild