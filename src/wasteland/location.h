// Wasteland Infinite additions. See LICENSE.md and upstream.lock.json.
#ifndef WASTELAND_LOCATION_H
#define WASTELAND_LOCATION_H

#include <cstdint>
#include <string>
#include <vector>

namespace wasteland {

// Versioned, platform-independent SplitMix64 streams; never uses Fallout's RNG.
class RandomStream {
public:
    RandomStream(uint64_t seed, const std::string& name);
    uint64_t next();
    uint64_t bounded(uint64_t bound);

private:
    uint64_t state_;
};

struct LocationTemplate {
    std::string id;
    std::string mapName;
    int elevation = 0;
    int entrance = 20100;
    int exit = 20100;
    std::vector<int> walkable;
    std::vector<int> obstacles;
    std::vector<int> reserved;
};

struct Placement {
    std::string id;
    int hex;
    std::string prototype; // symbolic allowlist, resolved through engine enums
};

struct Location {
    uint64_t seed;
    std::string id;
    LocationTemplate terrain;
    std::vector<Placement> placements;
};

// Verified against tileInit's _dir_tile for Fallout's 200 x 200 hex grid.
// Rejects row wrapping and edges, instead of accepting arbitrary linear offsets.
int neighbor(int hex, int rotation);
bool validHex(int hex);
LocationTemplate fixtureTemplate();
void validateTemplate(const LocationTemplate& terrain);
Location generate(uint64_t seed, const LocationTemplate& terrain);
void validate(const Location& location);
std::string toJson(const Location& location);
bool parseSeed(const std::string& text, uint64_t& result);

} // namespace wasteland
#endif
