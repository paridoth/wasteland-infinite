// Wasteland Infinite additions. Independent encounter RNG; native AI is separate.
#include "encounter.h"
#include <algorithm>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>

namespace wasteland {
namespace {
std::set<int> cells(const std::vector<int>& values)
{
    std::set<int> result;
    for (int cell : values) {
        if (!validHex(cell) || !result.insert(cell).second) throw std::invalid_argument("invalid or duplicate encounter cell");
    }
    return result;
}

std::set<int> radius(int entry, int depth)
{
    std::set<int> seen {entry};
    std::queue<std::pair<int, int>> pending;
    pending.push({entry, 0});
    while (!pending.empty()) {
        auto [cell, distance] = pending.front();
        pending.pop();
        if (distance == depth) continue;
        for (int dir = 0; dir < 6; ++dir) {
            int next = neighbor(cell, dir);
            if (next >= 0 && seen.insert(next).second) pending.push({next, distance + 1});
        }
    }
    return seen;
}

bool connected(std::set<int> remaining, int entry)
{
    if (!remaining.erase(entry)) return false;
    std::queue<int> pending;
    pending.push(entry);
    while (!pending.empty()) {
        int cell = pending.front();
        pending.pop();
        for (int dir = 0; dir < 6; ++dir) {
            int next = neighbor(cell, dir);
            if (remaining.erase(next)) pending.push(next);
        }
    }
    return remaining.empty();
}

LayoutInput encounterInput(const AlcovePlan& alcove)
{
    validateAlcove(alcove);
    LayoutInput input = alcove.input;
    for (int cell : alcove.blockers) input.walkable.erase(std::remove(input.walkable.begin(), input.walkable.end(), cell), input.walkable.end());
    input.protectedCells.push_back(alcove.opening);
    for (const auto& loot : alcove.loot) input.protectedCells.push_back(loot.hex);
    std::sort(input.walkable.begin(), input.walkable.end());
    std::sort(input.protectedCells.begin(), input.protectedCells.end());
    input.protectedCells.erase(std::unique(input.protectedCells.begin(), input.protectedCells.end()), input.protectedCells.end());
    return input;
}

void array(std::ostringstream& out, const std::vector<int>& values)
{
    out << '[';
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        out << values[i];
    }
    out << ']';
}
} // namespace

LayoutInput fixtureEncounter()
{
    auto patch = radius(20100, 4);
    return {20100, {patch.begin(), patch.end()}, {19898, 20099, 20100}};
}

void validateEncounter(const EncounterPlan& plan)
{
    if (plan.input.walkable.size() < 5 || plan.input.walkable.size() > 127 || plan.input.protectedCells.size() > 127) {
        throw std::invalid_argument("encounter requires 5..127 local cells");
    }
    auto remaining = cells(plan.input.walkable);
    auto protection = cells(plan.input.protectedCells);
    if (!protection.count(plan.input.entrance)
        || !std::includes(remaining.begin(), remaining.end(), protection.begin(), protection.end())
        || !connected(remaining, plan.input.entrance)) throw std::invalid_argument("invalid encounter protection or connectivity");
    if (!remaining.erase(plan.enemyHex) || protection.count(plan.enemyHex)
        || radius(plan.input.entrance, 3).count(plan.enemyHex)) throw std::invalid_argument("encounter overlaps protected cells or entry clearance");
    if (!connected(remaining, plan.input.entrance)) throw std::invalid_argument("encounter disconnects local movement");
}

void validateEncounter(const EncounterPlan& plan, const AlcovePlan& alcove)
{
    validateEncounter(plan);
    auto expected = encounterInput(alcove);
    if (plan.seed != alcove.seed || plan.input.entrance != expected.entrance
        || cells(plan.input.walkable) != cells(expected.walkable)
        || cells(plan.input.protectedCells) != cells(expected.protectedCells)) throw std::invalid_argument("encounter does not match its alcove");
}

EncounterPlan generateEncounter(const AlcovePlan& alcove)
{
    auto input = encounterInput(alcove);
    auto nearby = radius(input.entrance, 3);
    std::vector<int> candidates;
    for (int cell : input.walkable) {
        if (!nearby.count(cell) && !std::binary_search(input.protectedCells.begin(), input.protectedCells.end(), cell)) candidates.push_back(cell);
    }
    RandomStream stream(alcove.seed, "encounters/alcove-guard.v1");
    for (size_t size = candidates.size(); size > 1; --size) std::swap(candidates[size - 1], candidates[stream.bounded(size)]);
    for (int cell : candidates) {
        EncounterPlan plan {alcove.seed, input, cell};
        try { validateEncounter(plan, alcove); return plan; } catch (const std::invalid_argument&) { }
    }
    throw std::invalid_argument("no connected encounter cell outside entry clearance");
}

std::string toJson(const EncounterPlan& plan)
{
    validateEncounter(plan);
    std::ostringstream out;
    out << "{\"schema_version\":1,\"generator_version\":1,\"encounter\":\"alcove-guard.v1\",\"layout\":\"grove-alcove.v1\",\"seed\":\""
        << plan.seed << "\",\"entry\":" << plan.input.entrance << ",\"walkable\":";
    array(out, plan.input.walkable);
    out << ",\"protected\":";
    array(out, plan.input.protectedCells);
    out << ",\"entry_clearance\":4,\"enemy\":{\"hex\":" << plan.enemyHex << ",\"prototype\":\"template.hostile-ant\"}}\n";
    return out.str();
}
} // namespace wasteland
