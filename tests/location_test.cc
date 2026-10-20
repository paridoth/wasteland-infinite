// Wasteland Infinite asset-free core tests. Check macros stay active in Release.
#include "wasteland/location.h"
#include "wasteland/layout.h"
#include "wasteland/alcove.h"
#include "wasteland/encounter.h"
#include "wasteland/options.h"
#include <algorithm>
#include <functional>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace wasteland;
#define CHECK(condition) do { if (!(condition)) throw std::runtime_error(#condition); } while (0)
static void rejects(const std::function<void()>& f)
{
    bool caught = false;
    try { f(); } catch (const std::invalid_argument&) { caught = true; }
    CHECK(caught);
}

int main()
{
    try {
        uint64_t parsed;
        for (auto text : { "", "-1", "+1", " 1", "1 ", "1x", "01", "18446744073709551616" }) CHECK(!parseSeed(text, parsed));
        CHECK(parseSeed("18446744073709551615", parsed) && parsed == UINT64_MAX);
        CHECK(parseSeed("0", parsed) && parsed == 0);

        CHECK(neighbor(20100, 0) == 20099);
        CHECK(neighbor(20101, 0) == 19900);
        CHECK(neighbor(200, 0) == -1); // no wrapping column 0 to column 199
        CHECK(neighbor(-1, 0) == -1 && neighbor(40000, 0) == -1 && neighbor(20000, 6) == -1);
        for (int cell = 0; cell < 40000; ++cell) {
            for (int dir = 0; dir < 6; ++dir) {
                int next = neighbor(cell, dir);
                if (next >= 0) CHECK(neighbor(next, (dir + 3) % 6) == cell);
            }
        }

        RandomStream maps(42, "maps"), replay(42, "maps"), loot(42, "loot");
        for (int i = 0; i < 1000; ++i) {
            loot.next();
            CHECK(maps.next() == replay.next());
            CHECK(loot.bounded(3) < 3);
        }
        CHECK(RandomStream(42, "maps").next() != RandomStream(42, "topology").next());
        CHECK(RandomStream(42, "loot").next() != RandomStream(42, "encounters").next());
        rejects([&] { loot.bounded(0); });

        auto terrain = fixtureTemplate();
        for (uint64_t seed : { UINT64_C(0), UINT64_C(1), UINT64_C(42), UINT64_C(9007199254740993), UINT64_MAX }) {
            auto location = generate(seed, terrain);
            CHECK(toJson(location) == toJson(generate(seed, terrain)));
            auto reversed = terrain;
            std::reverse(reversed.walkable.begin(), reversed.walkable.end());
            std::reverse(reversed.obstacles.begin(), reversed.obstacles.end());
            CHECK(toJson(location) == toJson(generate(seed, reversed)));
        }
        CHECK(toJson(generate(1, terrain)) != toJson(generate(2, terrain)));

        auto layoutInput = fixtureLayout();
        CHECK(!visibleLayoutArt(1, 1, 0)); // native invisible blocking-marker regression
        CHECK(!visibleLayoutArt(300, 20, 100));
        CHECK(!visibleLayoutArt(10, 10, 101));
        CHECK(visibleLayoutArt(62, 141, 2108)); // visible native-source dimensions, no assets
        for (uint64_t seed : { UINT64_C(0), UINT64_C(1), UINT64_C(42), UINT64_C(9007199254740993), UINT64_MAX }) {
            auto plan = generateLayout(seed, layoutInput);
            validateLayout(plan);
            auto reversed = layoutInput;
            std::reverse(reversed.walkable.begin(), reversed.walkable.end());
            std::reverse(reversed.protectedCells.begin(), reversed.protectedCells.end());
            CHECK(toJson(plan) == toJson(generateLayout(seed, reversed)));
            plan.blockers[0] = plan.input.entrance;
            rejects([&] { validateLayout(plan); });
        }
        LayoutInput corridor {20100, {20100, 20101, 20102, 20103, 20104, 20105, 20106}, {20100, 20106}};
        rejects([&] { generateLayout(42, corridor); });
        auto invalidLayout = layoutInput;
        invalidLayout.walkable.push_back(40000);
        rejects([&] { generateLayout(42, invalidLayout); });
        invalidLayout = layoutInput;
        invalidLayout.protectedCells = invalidLayout.walkable;
        rejects([&] { generateLayout(42, invalidLayout); });
        invalidLayout = layoutInput;
        invalidLayout.walkable.push_back(invalidLayout.walkable.front());
        rejects([&] { generateLayout(42, invalidLayout); });

        // A real enclosure: only its declared opening neighbors the free interior.
        for (uint64_t seed : { UINT64_C(0), UINT64_C(1), UINT64_C(42), UINT64_C(9007199254740993), UINT64_MAX }) {
            auto plan = generateAlcove(seed, layoutInput);
            CHECK(plan.loot.size() == 3 && plan.loot[0].hex == plan.interior);
            int freeNeighbors = 0;
            for (int dir = 0; dir < 6; ++dir) {
                int next = neighbor(plan.interior, dir);
                if (std::find(plan.blockers.begin(), plan.blockers.end(), next) == plan.blockers.end()) {
                    CHECK(next == plan.opening);
                    ++freeNeighbors;
                }
            }
            CHECK(freeNeighbors == 1);
            auto reversed = layoutInput;
            std::reverse(reversed.walkable.begin(), reversed.walkable.end());
            std::reverse(reversed.protectedCells.begin(), reversed.protectedCells.end());
            CHECK(toJson(plan) == toJson(generateAlcove(seed, reversed)));
            auto invalid = plan;
            invalid.loot[1].hex = plan.opening;
            rejects([&] { validateAlcove(invalid); });
            invalid = plan;
            invalid.blockers[0] = plan.opening;
            rejects([&] { validateAlcove(invalid); });
            invalid = plan;
            invalid.input.protectedCells.push_back(plan.interior);
            rejects([&] { validateAlcove(invalid); });
        }
        rejects([&] { generateAlcove(42, corridor); });
        invalidLayout = layoutInput;
        invalidLayout.protectedCells = invalidLayout.walkable;
        rejects([&] { generateAlcove(42, invalidLayout); });

        for (uint64_t seed : { UINT64_C(0), UINT64_C(1), UINT64_C(42), UINT64_C(9007199254740993), UINT64_MAX }) {
            auto patch = fixtureEncounter();
            auto alcove = generateAlcove(seed, patch);
            auto encounter = generateEncounter(alcove);
            validateEncounter(encounter, alcove);
            CHECK(std::find(encounter.input.protectedCells.begin(), encounter.input.protectedCells.end(), encounter.enemyHex) == encounter.input.protectedCells.end());
            std::reverse(patch.walkable.begin(), patch.walkable.end());
            std::reverse(patch.protectedCells.begin(), patch.protectedCells.end());
            CHECK(toJson(encounter) == toJson(generateEncounter(generateAlcove(seed, patch))));
            auto invalid = encounter;
            invalid.enemyHex = alcove.opening;
            rejects([&] { validateEncounter(invalid); });
            invalid = encounter;
            invalid.seed ^= 1;
            rejects([&] { validateEncounter(invalid, alcove); });
            invalid = encounter;
            invalid.input.protectedCells.push_back(encounter.enemyHex);
            rejects([&] { validateEncounter(invalid, alcove); });
        }
        // Existing radius-three fixture cannot fit an enemy four hexes away.
        rejects([&] { generateEncounter(generateAlcove(42, fixtureLayout())); });
        EncounterPlan narrow {42, {20100, {20100, 20101, 20102, 20103, 20104, 20105}, {20100, 20105}}, 20104};
        rejects([&] { validateEncounter(narrow); });

        auto invalid = terrain;
        invalid.walkable.push_back(40000);
        rejects([&] { generate(1, invalid); });
        invalid = terrain;
        invalid.walkable.push_back(invalid.walkable[0]);
        rejects([&] { generate(1, invalid); });
        invalid = terrain;
        invalid.obstacles.push_back(invalid.entrance);
        rejects([&] { generate(1, invalid); });
        invalid = terrain;
        invalid.walkable.push_back(1000);
        rejects([&] { generate(1, invalid); });
        invalid = terrain;
        invalid.exit = 1000;
        rejects([&] { generate(1, invalid); });
        invalid = terrain;
        invalid.elevation = 3;
        rejects([&] { generate(1, invalid); });
        invalid = terrain;
        invalid.reserved = invalid.walkable;
        rejects([&] { generate(1, invalid); });

        auto location = generate(42, terrain);
        location.placements[0].prototype = "invented.object";
        rejects([&] { validate(location); });
        location = generate(42, terrain);
        location.placements[0].hex = location.placements[1].hex;
        rejects([&] { validate(location); });
        location = generate(42, terrain);
        location.placements[0].id = "missing.entity";
        rejects([&] { validate(location); });
        location = generate(42, terrain);
        location.placements[0].hex = terrain.exit;
        rejects([&] { validate(location); });

        char program[] = "engine";
        char inspect[] = "--wi-inspect=42";
        char develop[] = "--wi-dev-seed=42";
        char copied[] = "--wi-template-seed=42";
        char layoutArg[] = "--wi-layout-seed=42";
        char layoutInspect[] = "--wi-inspect-layout=42";
        char layoutProbe[] = "--wi-probe-layout=42";
        char load[] = "--dev-load-game=1";
        char* one[] = { program };
        char* two[] = { program, inspect };
        char* three[] = { program, develop, load };
        CHECK(parseOptions(1, one).mode == Mode::Disabled);
        CHECK(parseOptions(2, two).seed == 42);
        char* copiedArgs[] = { program, copied };
        CHECK(parseOptions(2, copiedArgs).mode == Mode::CopiedTemplate);
        char* layoutArgs[] = { program, layoutArg };
        char* layoutInspector[] = { program, layoutInspect };
        CHECK(parseOptions(2, layoutArgs).mode == Mode::Layout);
        CHECK(parseOptions(2, layoutInspector).mode == Mode::InspectLayout);
        char* layoutProber[] = { program, layoutProbe };
        CHECK(parseOptions(2, layoutProber).mode == Mode::ProbeLayout);
        char alcoveInspect[] = "--wi-inspect-alcove=18446744073709551615";
        char alcovePlay[] = "--wi-alcove-seed=42";
        char alcoveProbe[] = "--wi-probe-alcove=42";
        char* alcoveArgs[] = {program, alcoveInspect};
        CHECK(parseOptions(2, alcoveArgs).mode == Mode::InspectAlcove);
        alcoveArgs[1] = alcovePlay;
        CHECK(parseOptions(2, alcoveArgs).mode == Mode::Alcove);
        alcoveArgs[1] = alcoveProbe;
        CHECK(parseOptions(2, alcoveArgs).mode == Mode::ProbeAlcove);
        char encounterInspect[] = "--wi-inspect-encounter=42";
        char encounterPlay[] = "--wi-encounter-seed=42";
        char encounterProbe[] = "--wi-probe-encounter=42";
        char* encounterArgs[] = {program, encounterInspect};
        CHECK(parseOptions(2, encounterArgs).mode == Mode::InspectEncounter);
        encounterArgs[1] = encounterPlay;
        CHECK(parseOptions(2, encounterArgs).mode == Mode::Encounter);
        encounterArgs[1] = encounterProbe;
        CHECK(parseOptions(2, encounterArgs).mode == Mode::ProbeEncounter);
        rejects([&] { parseOptions(3, three); });

        // Only temporary files created by this test are ever removed.
        namespace fs = std::filesystem;
        auto root = fs::temp_directory_path() / ("wi-guard-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        CHECK(fs::create_directory(root));
        struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path, ec); } } cleanup { root };
        rejects([&] { validateRuntimeDirectory(root); });
        for (const auto& name : { "master.dat", "critter.dat" }) std::ofstream(root / name) << "synthetic test file";
        std::ofstream(root / ".wasteland-runtime") << "WastelandInfinite sandbox v1\n";
        fs::create_directory(root / "data");
        const std::string config = "[system]\nmaster_dat=master.dat\ncritter_dat=critter.dat\nmaster_patches=data\ncritter_patches=data\n";
        std::ofstream(root / "fallout2.cfg") << config;
        validateRuntimeDirectory(root);
        std::ofstream(root / "fallout2.cfg") << config << "[SYSTEM]\nMASTER_PATCHES=/tmp\n";
        rejects([&] { validateRuntimeDirectory(root); });
        std::ofstream(root / "fallout2.cfg") << config;
        fs::create_directory_symlink(root, root / "data/unsafe-link");
        rejects([&] { validateRuntimeDirectory(root); });
        fs::remove(root / "data/unsafe-link");
        rejects([&] { validateCopiedTemplateDirectory(root); });
        fs::create_directories(root / "data/maps");
        fs::create_directory(root / "template-source");
        std::ofstream(root / ".wasteland-template-copy") << "WastelandInfinite art temple copy v1\n";
        for (const auto& name : { "artemple.map", "arcaves.map" }) {
            std::ofstream(root / "data/maps" / name) << "synthetic map bytes";
            std::ofstream(root / "template-source" / name) << "synthetic map bytes";
        }
        validateCopiedTemplateDirectory(root);
        std::ofstream(root / "data/maps/artemple.map") << "modified template";
        rejects([&] { validateCopiedTemplateDirectory(root); });
        std::ofstream(root / "data/maps/artemple.map") << "synthetic map bytes";
        fs::remove(root / "template-source/arcaves.map");
        fs::create_symlink(root / "data/maps/arcaves.map", root / "template-source/arcaves.map");
        rejects([&] { validateCopiedTemplateDirectory(root); });
        std::cout << "PASS: seed bounds, all-grid neighbor reciprocity, named streams, replay, canonical order, connectivity, references, collisions, options\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
