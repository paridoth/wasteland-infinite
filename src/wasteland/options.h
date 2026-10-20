// Wasteland Infinite additions. See LICENSE.md.
#ifndef WASTELAND_OPTIONS_H
#define WASTELAND_OPTIONS_H
#include <cstdint>
#include <filesystem>
namespace wasteland {
enum class Mode { Disabled, Inspect, Develop, CopiedTemplate, InspectLayout, Layout, ProbeLayout, InspectAlcove, Alcove, ProbeAlcove, InspectEncounter, Encounter, ProbeEncounter };
struct Options {
    Mode mode = Mode::Disabled;
    uint64_t seed = 0;
};
Options parseOptions(int argc, char** argv);
void validateRuntimeDirectory(const std::filesystem::path& root);
void validateCopiedTemplateDirectory(const std::filesystem::path& root);
} // namespace wasteland
#endif
