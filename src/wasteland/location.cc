// Wasteland Infinite additions. See LICENSE.md and upstream.lock.json.
#include "location.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <iomanip>
#include <limits>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>

namespace wasteland {
namespace {
uint64_t hashName(const std::string& name)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char ch : name) {
        hash = (hash ^ ch) * UINT64_C(1099511628211);
    }
    return hash;
}

std::string hex64(uint64_t value)
{
    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(16) << value;
    return out.str();
}

std::string quote(const std::string& value)
{
    std::ostringstream out;
    out << '"';
    for (unsigned char ch : value) {
        if (ch == '"' || ch == '\\') {
            out << '\\' << ch;
        } else if (ch < 32 || ch >= 127) {
            out << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << int(ch);
        } else {
            out << ch;
        }
    }
    out << '"';
    return out.str();
}

std::string arrayJson(const std::vector<int>& values)
{
    std::ostringstream out;
    out << '[';
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        out << values[i];
    }
    out << ']';
    return out.str();
}

std::set<int> checkedSet(const std::vector<int>& values)
{
    if (values.size() > 40000) throw std::invalid_argument("hex collection exceeds grid");
    std::set<int> result;
    for (int value : values) {
        if (!validHex(value) || !result.insert(value).second) {
            throw std::invalid_argument("invalid or duplicate hex");
        }
    }
    return result;
}

bool validIdentifier(const std::string& id)
{
    return !id.empty() && id.size() <= 80 && std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-';
    });
}

std::string locationId(uint64_t seed, const LocationTemplate& terrain)
{
    return "loc-" + hex64(seed) + "-" + terrain.id;
}
} // namespace

RandomStream::RandomStream(uint64_t seed, const std::string& name)
    : state_(seed ^ hashName(name))
{
}

uint64_t RandomStream::next()
{
    uint64_t z = (state_ += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

uint64_t RandomStream::bounded(uint64_t bound)
{
    if (bound == 0) throw std::invalid_argument("zero RNG bound");
    uint64_t threshold = (uint64_t(0) - bound) % bound;
    uint64_t value;
    do { value = next(); } while (value < threshold);
    return value % bound;
}

bool validHex(int hex)
{
    return hex >= 0 && hex < 40000;
}

int neighbor(int hex, int rotation)
{
    if (!validHex(hex) || rotation < 0 || rotation >= 6) return -1;
    static constexpr int offsets[2][6] = {
        { -1, 199, 200, 201, 1, -200 },
        { -201, -1, 200, 1, -199, -200 },
    };
    int next = hex + offsets[hex % 2][rotation];
    if (!validHex(next) || std::abs(next % 200 - hex % 200) > 1) return -1;
    return next;
}

LocationTemplate fixtureTemplate()
{
    LocationTemplate terrain;
    terrain.id = "fixture.courtyard.v1";
    terrain.mapName = "fixture-only";
    terrain.entrance = 20100;
    terrain.exit = 20500;
    for (int y = 96; y <= 104; ++y) {
        for (int x = 96; x <= 104; ++x) {
            int hex = y * 200 + x;
            if (x == 98 && y >= 97 && y <= 99) {
                terrain.obstacles.push_back(hex);
            } else {
                terrain.walkable.push_back(hex);
            }
        }
    }
    return terrain;
}

void validateTemplate(const LocationTemplate& terrain)
{
    if (!validIdentifier(terrain.id) || terrain.mapName.empty() || terrain.mapName.size() > 32
        || terrain.elevation < 0 || terrain.elevation > 2) {
        throw std::invalid_argument("invalid template identity or elevation");
    }
    auto cells = checkedSet(terrain.walkable);
    auto blocked = checkedSet(terrain.obstacles);
    auto reserved = checkedSet(terrain.reserved);
    if (!cells.count(terrain.entrance) || !cells.count(terrain.exit)) {
        throw std::invalid_argument("entrance or exit not walkable");
    }
    for (int cell : blocked) {
        if (cells.count(cell)) throw std::invalid_argument("obstacle overlaps walkable hex");
    }
    for (int cell : reserved) {
        if (!cells.count(cell)) throw std::invalid_argument("reserved hex not walkable");
    }
    size_t capacity = 0;
    for (int cell : cells) {
        if (cell != terrain.entrance && cell != terrain.exit && !reserved.count(cell)) ++capacity;
    }
    if (capacity < 3) throw std::invalid_argument("template lacks space for placements");
    std::set<int> seen { terrain.entrance };
    std::queue<int> pending;
    pending.push(terrain.entrance);
    while (!pending.empty()) {
        int cell = pending.front();
        pending.pop();
        for (int direction = 0; direction < 6; ++direction) {
            int next = neighbor(cell, direction);
            if (cells.count(next) && seen.insert(next).second) pending.push(next);
        }
    }
    if (seen.size() != cells.size()) throw std::invalid_argument("disconnected template");
}

Location generate(uint64_t seed, const LocationTemplate& terrain)
{
    validateTemplate(terrain);
    Location location { seed, locationId(seed, terrain), terrain, {} };
    // Canonicalize caller ordering before randomization and serialization.
    std::sort(location.terrain.walkable.begin(), location.terrain.walkable.end());
    std::sort(location.terrain.obstacles.begin(), location.terrain.obstacles.end());
    std::sort(location.terrain.reserved.begin(), location.terrain.reserved.end());
    std::vector<int> candidates;
    for (int cell : location.terrain.walkable) {
        if (cell != terrain.entrance && cell != terrain.exit
            && !std::binary_search(location.terrain.reserved.begin(), location.terrain.reserved.end(), cell)) {
            candidates.push_back(cell);
        }
    }
    RandomStream maps(seed, "maps/" + terrain.id);
    RandomStream loot(seed, "loot/" + terrain.id);
    for (size_t i = candidates.size(); i > 1; --i) {
        std::swap(candidates[i - 1], candidates[maps.bounded(i)]);
    }
    for (size_t i = 0; i < 3; ++i) {
        location.placements.push_back({ location.id + ".loot-" + std::to_string(i), candidates[i],
            loot.bounded(2) == 0 ? "vanilla.money" : "vanilla.stimpak" });
    }
    validate(location);
    return location;
}

void validate(const Location& location)
{
    validateTemplate(location.terrain);
    if (location.id != locationId(location.seed, location.terrain) || location.placements.size() != 3) {
        throw std::invalid_argument("location identity or placement count mismatch");
    }
    std::set<int> occupied(location.terrain.reserved.begin(), location.terrain.reserved.end());
    occupied.insert(location.terrain.entrance);
    occupied.insert(location.terrain.exit);
    std::set<std::string> ids;
    for (size_t i = 0; i < location.placements.size(); ++i) {
        const auto& p = location.placements[i];
        if (std::find(location.terrain.walkable.begin(), location.terrain.walkable.end(), p.hex) == location.terrain.walkable.end()
            || !occupied.insert(p.hex).second || !ids.insert(p.id).second
            || p.id != location.id + ".loot-" + std::to_string(i)
            || (p.prototype != "vanilla.money" && p.prototype != "vanilla.stimpak")) {
            throw std::invalid_argument("invalid placement, reference, or overlap");
        }
    }
}

std::string toJson(const Location& location)
{
    validate(location);
    const auto& t = location.terrain;
    std::ostringstream out;
    out << "{\"schema_version\":1,\"generator_version\":1,\"seed\":" << quote(std::to_string(location.seed))
        << ",\"id\":" << quote(location.id) << ",\"type\":\"template-loot-proof\",\"biome\":\"inherited\""
        << ",\"template\":{\"id\":" << quote(t.id) << ",\"map\":" << quote(t.mapName)
        << ",\"floor\":\"inherit\",\"roof\":\"inherit\"},\"elevation\":" << t.elevation
        << ",\"walkable\":" << arrayJson(t.walkable) << ",\"obstacles\":" << arrayJson(t.obstacles)
        << ",\"reserved\":" << arrayJson(t.reserved)
        << ",\"entrances\":[" << t.entrance << "],\"exits\":[" << t.exit << ']'
        << ",\"buildings\":[],\"rooms\":[],\"doors\":[],\"transitions\":[],\"npcs\":[],\"anchors\":[]"
        << ",\"spawns\":[";
    for (size_t i = 0; i < location.placements.size(); ++i) {
        if (i) out << ',';
        const auto& p = location.placements[i];
        out << "{\"id\":" << quote(p.id) << ",\"hex\":" << p.hex << ",\"prototype\":" << quote(p.prototype) << '}';
    }
    out << "]}\n";
    return out.str();
}

bool parseSeed(const std::string& text, uint64_t& result)
{
    if (text.empty() || text.size() > 20 || (text.size() > 1 && text[0] == '0')) return false;
    auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    return parsed.ec == std::errc() && parsed.ptr == text.data() + text.size();
}
} // namespace wasteland
