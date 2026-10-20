// Wasteland Infinite additions. Runtime evidence: docs/wasteland/VALIDATION.md.
#include "runtime.h"
#include "location.h"
#include "layout.h"
#include "alcove.h"
#include "encounter.h"
#include "encounter_runtime.h"
#include "options.h"

#include <cstdio>
#include <algorithm>
#include <fstream>
#include <array>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <tuple>

#include "animation.h"
#include "art.h"
#include "debug.h"
#include "display_monitor.h"
#include "map.h"
#include "object.h"
#include "proto.h"
#include "proto_types.h"
#include "tile.h"

namespace wasteland {
namespace {
Options options;
bool placementOk = false;
bool layoutMode() { return options.mode == Mode::Layout || options.mode == Mode::ProbeLayout; }
bool encounterMode() { return options.mode == Mode::Encounter || options.mode == Mode::ProbeEncounter; }
bool alcoveMode() { return options.mode == Mode::Alcove || options.mode == Mode::ProbeAlcove || encounterMode(); }
bool structureMode() { return layoutMode() || alcoveMode(); }
static_assert(HEX_GRID_WIDTH == 200 && HEX_GRID_HEIGHT == 200, "review location contract for changed grid");

fallout::ProtoId resolve(const std::string& name)
{
    if (name == "vanilla.money") return fallout::ItemProtoTypeId::Money;
    if (name == "vanilla.stimpak") return fallout::ItemProtoTypeId::Stimpak;
    throw std::invalid_argument("unapproved prototype");
}

// Read-only, bounded audit of native exit objects. Never invents a transition or
// moves the player to test it. Collect first: pathfinding uses object iterators.
std::vector<int> auditCopiedTemplate()
{
    using namespace fallout;
    std::vector<Object*> exits;
    for (Object* object = objectFindFirst(); object != nullptr; object = objectFindNext()) {
        if (isExitGridProtoId(object)) {
            if (exits.size() == 128) throw std::runtime_error("too many template exit grids");
            exits.push_back(object);
        }
    }
    std::sort(exits.begin(), exits.end(), [](const Object* a, const Object* b) {
        return std::pair<int, int>(a->elevation, a->tile) < std::pair<int, int>(b->elevation, b->tile);
    });
    std::ostringstream report;
    report << "{\"schema_version\":1,\"template_id\":\"artemple-copy.v1\",\"map\":\"ARTEMPLE.MAP\","
           << "\"entry_hex\":" << gDude->tile << ",\"entry_elevation\":" << gDude->elevation
           << ",\"entry_rotation\":" << gDude->rotation << ",\"exits\":[";
    int reachable = 0;
    std::vector<int> reachableExits;
    for (size_t index = 0; index < exits.size(); ++index) {
        const auto* object = exits[index];
        const auto& target = object->data.misc;
        char mapName[32] = {};
        bool referenceValid = wmMapIdxToName(target.map, mapName, sizeof(mapName)) == 0;
        // Registry names are native identifiers; reject control/quote/path chars.
        for (const char* p = mapName; *p; ++p) {
            if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')
                    || (*p >= '0' && *p <= '9') || *p == '_' || *p == '.')) {
                throw std::runtime_error("invalid destination map identifier");
            }
        }
        if (referenceValid) {
            std::string path = std::string("maps\\") + mapName;
            File* file = fileOpen(path.c_str(), "rb");
            referenceValid = file != nullptr;
            if (file != nullptr) fileClose(file);
        }
        referenceValid = referenceValid && (validHex(target.tile) || target.tile == -1)
            && elevationIsValid(target.elevation) && rotationIsValid(target.rotation);
        bool hidden = (object->flags & OBJECT_HIDDEN) != OBJECT_NONE;
        int pathLength = -1; // untested: hidden, different elevation or invalid source
        if (!hidden && validHex(object->tile) && object->elevation == gDude->elevation) {
            pathLength = object->tile == gDude->tile ? 0
                : _make_path(gDude, gDude->tile, object->tile, nullptr, 1);
            if ((pathLength > 0 || object->tile == gDude->tile) && referenceValid) {
                ++reachable;
                reachableExits.push_back(object->tile);
            }
        }
        if (index) report << ',';
        report << "{\"hex\":" << object->tile << ",\"elevation\":" << object->elevation
               << ",\"hidden\":" << (hidden ? "true" : "false")
               << ",\"destination_map_index\":" << target.map << ",\"destination_map\":\"" << mapName
               << "\",\"destination_hex\":" << target.tile << ",\"destination_elevation\":" << target.elevation
               << ",\"destination_rotation\":" << target.rotation << ",\"reference_valid\":"
               << (referenceValid ? "true" : "false") << ",\"path_length\":" << pathLength << '}';
    }
    report << "],\"reachable_exit_count\":" << reachable << "}\n";
    std::ofstream output("wasteland-template-audit.json", std::ios::trunc);
    output << report.str();
    output.close();
    if (!output) throw std::runtime_error("cannot write template audit");
    if (reachable == 0) throw std::runtime_error("copied template has no currently reachable native exit with valid destination");
    debugPrint("WASTELAND TEMPLATE: audited %d native exit grids; %d reachable from entry\n", int(exits.size()), reachable);
    return reachableExits;
}

struct LayoutSource {
    int pid;
    int fid;
    int tile;
    fallout::Rotation rotation;
};

bool validLayoutFlags(unsigned flags)
{
    using namespace fallout;
    // A prop must persist in native saves and remain removable for rollback.
    return (flags & unsigned(OBJECT_HIDDEN | OBJECT_NO_BLOCK | OBJECT_MULTIHEX | OBJECT_NO_SAVE | OBJECT_NO_REMOVE)) == 0;
}

LayoutSource findLayoutSource()
{
    using namespace fallout;
    std::vector<Object*> candidates;
    for (Object* object = objectFindFirst(); object; object = objectFindNext()) {
        ObjectType type = ProtoId(object).objectType();
        if ((type == OBJ_TYPE_WALL || type == OBJ_TYPE_SCENERY)
            && object->elevation == gDude->elevation && validHex(object->tile)
            && tileDistanceBetween(gDude->tile, object->tile) <= 12) {
            if (candidates.size() == 4096) throw std::runtime_error("too many native layout sources");
            candidates.push_back(object);
        }
    }
    std::vector<LayoutSource> sources;
    for (Object* object : candidates) {
        if (nativeProbeRequested()) {
            Proto* p = nullptr;
            if (protoGetProto(object, &p) == 0 && p) {
                FrmImage frame;
                bool loaded = frame.lock(FrmId(object), 0, object->rotation);
                int pixels = loaded ? frame.getWidth() * frame.getHeight() : 0;
                int visible = loaded && pixels > 0 && pixels <= 65536
                    ? int(std::count_if(frame.getData(), frame.getData() + pixels, [](unsigned char c) { return c != 0; })) : 0;
                debugPrint("WI SOURCE: pid=%d fid=%d tile=%d flags=%u sid=%d inv=%d proto_fid=%d proto_flags=%u proto_sid=%d subtype=%d art=%dx%d visible=%d\n",
                    object->pid, object->fid, object->tile, unsigned(object->flags), object->sid,
                    object->data.inventory.length, p->fid, unsigned(p->flags), p->sid,
                    ProtoId(object).objectType() == OBJ_TYPE_SCENERY ? int(p->scenery.type) : -1,
                    loaded ? frame.getWidth() : 0, loaded ? frame.getHeight() : 0, visible);
            }
        }
        ObjectType type = ProtoId(object).objectType();
        if (ProtoId(object) == ProtoId(SceneryProtoTypeId::BlockingHexAutomap)
            || ProtoId(object) == ProtoId(SceneryProtoTypeId::ExitGridAutomapMarker)) continue;
        if (object->sid != -1 || object->data.inventory.length != 0 || !validLayoutFlags(unsigned(object->flags))) continue;
        Proto* proto = nullptr;
        if (protoGetProto(object, &proto) != 0 || !proto || proto->sid != -1 || proto->fid != object->fid
            || !validLayoutFlags(unsigned(proto->flags))) continue;
        if (type == OBJ_TYPE_SCENERY && proto->scenery.type != SCENERY_TYPE_GENERIC) continue;
        FrmImage image;
        if (!image.lock(FrmId(object), 0, object->rotation)
            || image.getWidth() <= 0 || image.getHeight() <= 0
            || image.getWidth() > 256 || image.getHeight() > 256) continue;
        int pixels = image.getWidth() * image.getHeight();
        int visible = int(std::count_if(image.getData(), image.getData() + pixels,
            [](unsigned char color) { return color != 0; }));
        if (!visibleLayoutArt(image.getWidth(), image.getHeight(), visible)) continue;
        sources.push_back({object->pid, object->fid, object->tile, object->rotation});
    }
    if (sources.empty()) throw std::runtime_error("no verified scriptless single-hex native prop for partition");
    auto key = [](const LayoutSource& source) {
        return std::make_tuple(fallout::tileDistanceBetween(fallout::gDude->tile, source.tile),
            source.pid, source.tile, source.rotation);
    };
    std::sort(sources.begin(), sources.end(), [&](const auto& a, const auto& b) { return key(a) < key(b); });
    debugPrint("WASTELAND LAYOUT SOURCE: %s\n", protoGetName(ProtoId(sources.front().pid)));
    return sources.front();
}

std::vector<int> protectNativeRoutes(const LocationTemplate& terrain, const std::vector<int>& exits)
{
    using namespace fallout;
    std::set<int> routes { terrain.entrance };
    for (int exit : exits) {
        if (exit == terrain.entrance) continue;
        std::array<unsigned char, 800> rotations {};
        int length = _make_path(gDude, terrain.entrance, exit, rotations.data(), 1);
        if (length <= 0 || length >= int(rotations.size())) throw std::runtime_error("cannot reserve native exit route");
        int cell = terrain.entrance;
        for (int step = 0; step < length; ++step) {
            cell = tileGetTileInDirection(cell, static_cast<Rotation>(rotations[step]), 1);
            if (!validHex(cell)) throw std::runtime_error("invalid native exit route");
            routes.insert(cell);
        }
        if (cell != exit) throw std::runtime_error("native route reconstruction mismatch");
    }
    std::vector<int> result;
    for (int cell : terrain.walkable) if (routes.count(cell)) result.push_back(cell);
    return result;
}
} // namespace

bool nativeProbeRequested() { return options.mode == Mode::ProbeLayout || options.mode == Mode::ProbeAlcove || options.mode == Mode::ProbeEncounter; }
bool placementSucceeded() { return placementOk; }
void resetPlacementStatus() { placementOk = false; }
bool layoutGameplayRequested() { return options.mode == Mode::Layout || options.mode == Mode::Alcove || options.mode == Mode::Encounter; }
bool skipNewGameIntro() { return options.mode != Mode::Disabled; }

int preflight(int argc, char** argv)
{
    try {
        options = parseOptions(argc, argv);
        if (options.mode == Mode::Inspect) {
            std::fputs(toJson(generate(options.seed, fixtureTemplate())).c_str(), stdout);
            return 0;
        }
        if (options.mode == Mode::InspectLayout) {
            std::fputs(toJson(generateLayout(options.seed, fixtureLayout())).c_str(), stdout);
            return 0;
        }
        if (options.mode == Mode::InspectAlcove) {
            std::fputs(toJson(generateAlcove(options.seed, fixtureLayout())).c_str(), stdout);
            return 0;
        }
        if (options.mode == Mode::InspectEncounter) {
            std::fputs(toJson(generateEncounter(generateAlcove(options.seed, fixtureEncounter()))).c_str(), stdout);
            return 0;
        }
        if (options.mode == Mode::Develop) {
            validateRuntimeDirectory(std::filesystem::current_path());
        } else if (options.mode == Mode::CopiedTemplate || structureMode()) {
            validateCopiedTemplateDirectory(std::filesystem::current_path());
        }
        return -1;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Wasteland Infinite: %s\n", error.what());
        return 2;
    }
}

void populateNewGame()
{
    if (options.mode != Mode::Develop && options.mode != Mode::CopiedTemplate && !structureMode()) return;
    placementOk = false;
    using namespace fallout;
    std::vector<Object*> created;
    try {
        // Provisional template: the upstream new-game map only. Never on loads,
        // transitions, saved map overrides, or a conversion's custom start map.
        if (std::string(gMapHeader.name) != "ARTEMPLE.MAP"
            || (gMapHeader.flags & MAP_HEADER_SAVED) != MAP_HEADER_NONE
            || gDude == nullptr || !validHex(gDude->tile)) {
            throw std::runtime_error("proof requires a fresh vanilla ARTEMPLE.MAP");
        }
        std::vector<int> nativeExits;
        if (options.mode == Mode::CopiedTemplate || structureMode()) nativeExits = auditCopiedTemplate();
        LocationTemplate terrain;
        terrain.id = "engine.artemple.v1";
        terrain.mapName = "ARTEMPLE.MAP";
        terrain.elevation = static_cast<int>(gDude->elevation);
        terrain.entrance = gDude->tile;
        // This proof's exit is the return-to-start anchor, not a new exit grid.
        terrain.exit = gDude->tile;
        terrain.walkable.push_back(gDude->tile);
        std::set<int> seen { gDude->tile };
        std::queue<std::pair<int, int>> pending;
        pending.push({ gDude->tile, 0 });
        while (!pending.empty()) {
            auto [cell, depth] = pending.front();
            pending.pop();
            if (depth >= (alcoveMode() ? 6 : 4)) continue; // alcove <=127; existing proofs <=61
            for (int dir = 0; dir < 6; ++dir) {
                int next = neighbor(cell, dir);
                if (next < 0 || !seen.insert(next).second) continue;
                if (tileIsEdge(cell) || tileIsEdge(next)
                    || tileGetTileInDirection(cell, static_cast<Rotation>(dir), 1) != next) continue;
                if (objectFindFirstAtLocation(gDude->elevation, next) != nullptr
                    || _obj_blocking_at(gDude, next, gDude->elevation) != nullptr
                    || _make_path(gDude, gDude->tile, next, nullptr, 1) <= 0) continue;
                terrain.walkable.push_back(next);
                pending.push({ next, depth + 1 });
            }
        }
        std::optional<LayoutPlan> layout;
        std::optional<AlcovePlan> alcove;
        std::optional<EncounterPlan> encounter;
        std::optional<EncounterSource> encounterSource;
        std::optional<LayoutSource> layoutSource;
        std::vector<int> blockers;
        if (structureMode()) {
            LayoutInput input {terrain.entrance, terrain.walkable, protectNativeRoutes(terrain, nativeExits)};
            if (alcoveMode()) {
                alcove = generateAlcove(options.seed, input);
                blockers = alcove->blockers;
                terrain.id = "engine.artemple.alcove.v1";
                terrain.reserved = input.protectedCells;
                terrain.reserved.push_back(alcove->opening);
                std::sort(terrain.reserved.begin(), terrain.reserved.end());
                terrain.reserved.erase(std::unique(terrain.reserved.begin(), terrain.reserved.end()), terrain.reserved.end());
            } else {
                layout = generateLayout(options.seed, input);
                blockers = layout->blockers;
                terrain.id = "engine.artemple.partition.v1";
                terrain.reserved = input.protectedCells;
            }
            layoutSource = findLayoutSource();
            for (int cell : blockers) {
                terrain.walkable.erase(std::remove(terrain.walkable.begin(), terrain.walkable.end(), cell), terrain.walkable.end());
                terrain.obstacles.push_back(cell);
            }
        }
        if (encounterMode()) {
            encounter = generateEncounter(*alcove);
            encounterSource = findEncounterSource(nativeProbeRequested());
        }
        std::optional<Location> location;
        std::vector<AlcoveLoot> placements;
        if (alcove) {
            placements = alcove->loot;
        } else {
            location = generate(options.seed, terrain);
            for (const auto& item : location->placements) placements.push_back({item.hex, item.prototype});
        }
        // Validate prototypes and loadable art before any world mutation.
        for (const auto& p : placements) {
            Proto* proto = nullptr;
            if (protoGetProto(resolve(p.prototype), &proto) != 0 || proto == nullptr
                || proto->sid != -1) throw std::runtime_error("missing or scripted prototype");
            FrmImage image;
            if (!image.lock(FrmId(proto))) throw std::runtime_error("prototype art cannot load");
        }
        if (structureMode()) {
            for (int cell : blockers) {
                Object* object = nullptr;
                if (objectCreateWithProtoId(&object, ProtoId(layoutSource->pid)) != 0 || !object) {
                    throw std::runtime_error("partition object creation failed");
                }
                created.push_back(object);
                if (object->sid != -1 || object->fid != layoutSource->fid
                    || !validLayoutFlags(unsigned(object->flags))
                    || objectSetLocation(object, cell, gDude->elevation, nullptr) != 0
                    || objectSetRotation(object, layoutSource->rotation, nullptr) != 0
                    || _obj_blocking_at(gDude, cell, gDude->elevation) != object) {
                    throw std::runtime_error("partition is not a verified single-hex blocker");
                }
            }
            for (int cell : terrain.walkable) {
                if (cell != terrain.entrance && _make_path(gDude, terrain.entrance, cell, nullptr, 1) <= 0) {
                    throw std::runtime_error("partition disconnects native local movement");
                }
            }
            for (int exit : nativeExits) {
                if (exit != terrain.entrance && _make_path(gDude, terrain.entrance, exit, nullptr, 1) <= 0) {
                    throw std::runtime_error("partition blocks a previously reachable native exit");
                }
            }
        }
        for (const auto& p : placements) {
            Object* object = nullptr;
            if (objectCreateWithProtoId(&object, resolve(p.prototype)) != 0 || object == nullptr) {
                throw std::runtime_error("object creation failed");
            }
            created.push_back(object);
            if (objectSetLocation(object, p.hex, gDude->elevation, nullptr) != 0) {
                throw std::runtime_error("object placement failed");
            }
            object->flags |= OBJECT_NO_BLOCK; // loot cannot obstruct legacy routes
        }
        if (encounter) {
            createEncounterAnt(*encounterSource, encounter->enemyHex, created);
            // A living critter blocks its cell. Every other cell and native exit
            // must still be accessible before the player initiates combat.
            for (int cell : terrain.walkable) {
                if (cell != terrain.entrance && cell != encounter->enemyHex
                    && _make_path(gDude, terrain.entrance, cell, nullptr, 1) <= 0) {
                    throw std::runtime_error("encounter disconnects native local movement");
                }
            }
            for (int exit : nativeExits) {
                if (exit != terrain.entrance && _make_path(gDude, terrain.entrance, exit, nullptr, 1) <= 0) {
                    throw std::runtime_error("encounter blocks a previously reachable native exit");
                }
            }
        }
        // Diagnostic contract only; this file is NOT a campaign save/checkpoint.
        if (location) {
            std::ofstream report("wasteland-location.json", std::ios::trunc);
            report << toJson(*location);
            report.close();
            if (!report) throw std::runtime_error("cannot write placement diagnostic");
        }
        if (layout) {
            std::ofstream layoutReport("wasteland-layout.json", std::ios::trunc);
            layoutReport << toJson(*layout);
            layoutReport.close();
            if (!layoutReport) throw std::runtime_error("cannot write layout diagnostic");
            debugPrint("WASTELAND LAYOUT: source pid=%d fid=%d tile=%d rotation=%d; blockers=%d,%d; preserved native exits=%d\n",
                layoutSource->pid, layoutSource->fid, layoutSource->tile, layoutSource->rotation,
                layout->blockers[0], layout->blockers[1], int(nativeExits.size()));
            displayMonitorAddMessage("Wasteland Infinite: added a two-object partition; native exit paths preserved.");
        }
        if (alcove) {
            std::ofstream report("wasteland-alcove.json", std::ios::trunc);
            report << toJson(*alcove);
            report.close();
            if (!report) throw std::runtime_error("cannot write alcove diagnostic");
            debugPrint("WASTELAND ALCOVE: source pid=%d fid=%d tile=%d rotation=%d; interior=%d opening=%d blockers=%d,%d,%d,%d,%d; preserved native exits=%d\n",
                layoutSource->pid, layoutSource->fid, layoutSource->tile, layoutSource->rotation,
                alcove->interior, alcove->opening, blockers[0], blockers[1], blockers[2], blockers[3], blockers[4], int(nativeExits.size()));
            displayMonitorAddMessage("Wasteland Infinite: added a five-tree alcove with one opening and loot inside.");
        }
        if (encounter) {
            std::ofstream report("wasteland-encounter.json", std::ios::trunc);
            report << toJson(*encounter);
            report.close();
            if (!report) throw std::runtime_error("cannot write encounter diagnostic");
            debugPrint("WASTELAND ENCOUNTER: source pid=%d fid=%d tile=%d team=%d ai=%d; enemy=%d; preserved native exits=%d\n",
                encounterSource->pid, encounterSource->fid, encounterSource->tile, encounterSource->team,
                encounterSource->aiPacket, encounter->enemyHex, int(nativeExits.size()));
            displayMonitorAddMessage("Wasteland Infinite: added one seeded ant. Initiate combat to test native AI.");
        }
        tileWindowRefresh();
        displayMonitorAddMessage("Wasteland Infinite: placed 3 seeded loot objects near the entrance.");
        if (location) debugPrint("WASTELAND: %s\n", location->id.c_str());
        else debugPrint("WASTELAND: grove-alcove.v1 seed=%llu loot=3\n", static_cast<unsigned long long>(options.seed));
        placementOk = true;
    } catch (const std::exception& error) {
        for (Object* object : created) objectDestroy(object, nullptr);
        if (structureMode()) {
            std::error_code ignored;
            std::filesystem::remove("wasteland-layout.json", ignored);
            std::filesystem::remove("wasteland-location.json", ignored);
            std::filesystem::remove("wasteland-alcove.json", ignored);
            std::filesystem::remove("wasteland-encounter.json", ignored);
        }
        tileWindowRefresh();
        debugPrint("WASTELAND: placement cancelled: %s\n", error.what());
        displayMonitorAddMessage("Wasteland Infinite: placement cancelled; see debug log.");
    }
}
} // namespace wasteland
