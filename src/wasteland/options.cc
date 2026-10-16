// Wasteland Infinite additions. See LICENSE.md.
#include "options.h"
#include "location.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

namespace wasteland {
Options parseOptions(int argc, char** argv)
{
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg.rfind("--wi-", 0) != 0) continue;
        if (argc != 2) throw std::invalid_argument("use one --wi option alone; native saves/config overrides are not supported in the proof");
        const std::string inspect = "--wi-inspect=";
        const std::string develop = "--wi-dev-seed=";
        size_t prefix;
        if (arg.rfind(inspect, 0) == 0) {
            options.mode = Mode::Inspect;
            prefix = inspect.size();
        } else if (arg.rfind(develop, 0) == 0) {
            options.mode = Mode::Develop;
            prefix = develop.size();
        } else {
            throw std::invalid_argument("unknown Wasteland option");
        }
        if (!parseSeed(arg.substr(prefix), options.seed)) {
            throw std::invalid_argument("seed must be canonical unsigned decimal uint64");
        }
    }
    return options;
}

void validateRuntimeDirectory(const std::filesystem::path& root)
{
    namespace fs = std::filesystem;
    for (const auto& name : { ".wasteland-runtime", "fallout2.cfg", "master.dat", "critter.dat" }) {
        auto p = root / name;
        if (!fs::is_regular_file(p) || fs::is_symlink(p)) {
            throw std::invalid_argument("run only from a copy made by tools/wasteland/stage_runtime.py");
        }
    }
    std::ifstream marker(root / ".wasteland-runtime");
    std::string line;
    std::getline(marker, line);
    if (line != "WastelandInfinite sandbox v1") throw std::invalid_argument("invalid runtime marker");
    if (!fs::is_directory(root / "data") || fs::is_symlink(root / "data")) {
        throw std::invalid_argument("runtime data must be a separate directory");
    }
    size_t count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(root / "data")) {
        if (++count > 100000 || entry.is_symlink()) throw std::invalid_argument("unsafe runtime data tree");
    }
    std::map<std::string, std::string> expected {
        { "master_dat", "master.dat" }, { "critter_dat", "critter.dat" },
        { "master_patches", "data" }, { "critter_patches", "data" },
    };
    // Require all four paths explicitly, refusing duplicates and path redirection.
    std::ifstream config(root / "fallout2.cfg");
    bool system = false;
    while (std::getline(config, line)) {
        auto comment = line.find(';');
        if (comment != std::string::npos) line.erase(comment);
        auto trim = [](std::string s) {
            auto start = s.find_first_not_of(" \t\r");
            return start == std::string::npos ? std::string() : s.substr(start, s.find_last_not_of(" \t\r") - start + 1);
        };
        line = trim(line);
        auto lower = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
            return s;
        };
        if (!line.empty() && line[0] == '[') { system = lower(line) == "[system]"; continue; }
        auto equals = line.find('=');
        if (!system || equals == std::string::npos) continue;
        auto key = lower(trim(line.substr(0, equals)));
        if (key == "master_dat" || key == "critter_dat" || key == "master_patches" || key == "critter_patches") {
            auto found = expected.find(key);
            if (found == expected.end() || trim(line.substr(equals + 1)) != found->second) {
                throw std::invalid_argument("runtime config redirects or duplicates an asset/save path");
            }
            expected.erase(found);
        }
    }
    if (!expected.empty()) throw std::invalid_argument("runtime config is missing isolated paths");
}
} // namespace wasteland
