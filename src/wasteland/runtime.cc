// Wasteland Infinite additions. Runtime validation with owned assets is pending.
#include "runtime.h"
#include "location.h"
#include "options.h"

#include <cstdio>
#include <fstream>
#include <queue>
#include <set>
#include <stdexcept>
#include <vector>

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
static_assert(HEX_GRID_WIDTH == 200 && HEX_GRID_HEIGHT == 200, "review location contract for changed grid");

fallout::ProtoId resolve(const std::string& name)
{
    if (name == "vanilla.money") return fallout::ItemProtoTypeId::Money;
    if (name == "vanilla.stimpak") return fallout::ItemProtoTypeId::Stimpak;
    throw std::invalid_argument("unapproved prototype");
}
} // namespace

int preflight(int argc, char** argv)
{
    try {
        options = parseOptions(argc, argv);
        if (options.mode == Mode::Inspect) {
            std::fputs(toJson(generate(options.seed, fixtureTemplate())).c_str(), stdout);
            return 0;
        }
        if (options.mode == Mode::Develop) {
            validateRuntimeDirectory(std::filesystem::current_path());
        }
        return -1;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Wasteland Infinite: %s\n", error.what());
        return 2;
    }
}

void populateNewGame()
{
    if (options.mode != Mode::Develop) return;
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
            if (depth >= 4) continue; // at most 61 local hexes
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
        auto location = generate(options.seed, terrain);
        // Validate prototypes and loadable art before any world mutation.
        for (const auto& p : location.placements) {
            Proto* proto = nullptr;
            if (protoGetProto(resolve(p.prototype), &proto) != 0 || proto == nullptr
                || proto->sid != -1) throw std::runtime_error("missing or scripted prototype");
            FrmImage image;
            if (!image.lock(FrmId(proto))) throw std::runtime_error("prototype art cannot load");
        }
        for (const auto& p : location.placements) {
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
        // Diagnostic contract only; this file is NOT a campaign save/checkpoint.
        std::ofstream report("wasteland-location.json", std::ios::trunc);
        report << toJson(location);
        report.close();
        if (!report) throw std::runtime_error("cannot write placement diagnostic");
        tileWindowRefresh();
        displayMonitorAddMessage("Wasteland Infinite: placed 3 seeded loot objects near the entrance.");
        debugPrint("WASTELAND: %s\n", location.id.c_str());
    } catch (const std::exception& error) {
        for (Object* object : created) objectDestroy(object, nullptr);
        tileWindowRefresh();
        debugPrint("WASTELAND: placement cancelled: %s\n", error.what());
        displayMonitorAddMessage("Wasteland Infinite: placement cancelled; see debug log.");
    }
}
} // namespace wasteland
