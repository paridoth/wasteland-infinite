// Wasteland Infinite additions. Five props, one opening, one interior reward.
#include "alcove.h"
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
    for (int value : values) {
        if (!validHex(value) || !result.insert(value).second) throw std::invalid_argument("invalid or duplicate alcove cell");
    }
    return result;
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

void validateInput(const LayoutInput& input)
{
    if (input.walkable.size() < 10 || input.walkable.size() > 127 || input.protectedCells.size() > 127) {
        throw std::invalid_argument("alcove requires 10..127 local cells");
    }
    auto walkable = cells(input.walkable);
    auto protection = cells(input.protectedCells);
    if (!protection.count(input.entrance)
        || !std::includes(walkable.begin(), walkable.end(), protection.begin(), protection.end())
        || !connected(walkable, input.entrance)) throw std::invalid_argument("invalid alcove protection or connectivity");
}

// Returns available exterior loot cells after validating the complete enclosure.
std::vector<int> validateGeometry(const AlcovePlan& plan)
{
    validateInput(plan.input);
    auto remaining = cells(plan.input.walkable);
    auto protection = cells(plan.input.protectedCells);
    auto blocked = cells(plan.blockers);
    if (blocked.size() != 5 || !remaining.count(plan.interior) || protection.count(plan.interior)
        || !remaining.count(plan.opening) || plan.interior == plan.opening) {
        throw std::invalid_argument("invalid alcove interior or opening");
    }
    std::set<int> ring;
    for (int dir = 0; dir < 6; ++dir) {
        int next = neighbor(plan.interior, dir);
        if (!validHex(next)) throw std::invalid_argument("alcove touches grid edge");
        ring.insert(next);
    }
    if (!ring.erase(plan.opening) || ring != blocked) throw std::invalid_argument("alcove must have exactly one opening");
    for (int cell : blocked) {
        if (!remaining.erase(cell) || protection.count(cell)) throw std::invalid_argument("alcove blocks protected or unavailable terrain");
    }
    if (!connected(remaining, plan.input.entrance)) throw std::invalid_argument("alcove disconnects local movement");
    std::vector<int> exterior;
    for (int cell : remaining) {
        if (cell != plan.interior && cell != plan.opening && !protection.count(cell)) exterior.push_back(cell);
    }
    if (exterior.size() < 2) throw std::invalid_argument("alcove lacks exterior loot capacity");
    return exterior;
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

void validateAlcove(const AlcovePlan& plan)
{
    auto exterior = validateGeometry(plan);
    if (plan.loot.size() != 3 || plan.loot[0].hex != plan.interior) throw std::invalid_argument("alcove requires one interior and two exterior loot objects");
    std::set<int> occupied;
    for (size_t index = 0; index < plan.loot.size(); ++index) {
        const auto& item = plan.loot[index];
        if (!occupied.insert(item.hex).second
            || (index != 0 && !std::binary_search(exterior.begin(), exterior.end(), item.hex))
            || (item.prototype != "vanilla.money" && item.prototype != "vanilla.stimpak")) {
            throw std::invalid_argument("invalid alcove loot reference or overlap");
        }
    }
}

AlcovePlan generateAlcove(uint64_t seed, LayoutInput input)
{
    validateInput(input);
    std::sort(input.walkable.begin(), input.walkable.end());
    std::sort(input.protectedCells.begin(), input.protectedCells.end());
    std::vector<std::pair<int, int>> candidates;
    for (int center : input.walkable) {
        if (std::binary_search(input.protectedCells.begin(), input.protectedCells.end(), center)) continue;
        for (int dir = 0; dir < 6; ++dir) {
            int opening = neighbor(center, dir);
            if (validHex(opening)) candidates.push_back({center, opening});
        }
    }
    std::sort(candidates.begin(), candidates.end());
    RandomStream topology(seed, "rooms/grove-alcove.v1");
    for (size_t size = candidates.size(); size > 1; --size) std::swap(candidates[size - 1], candidates[topology.bounded(size)]);
    for (auto [center, opening] : candidates) {
        AlcovePlan plan { seed, input, center, opening, {}, {} };
        for (int dir = 0; dir < 6; ++dir) {
            int next = neighbor(center, dir);
            if (next != opening) plan.blockers.push_back(next);
        }
        std::sort(plan.blockers.begin(), plan.blockers.end());
        std::vector<int> exterior;
        try { exterior = validateGeometry(plan); } catch (const std::invalid_argument&) { continue; }
        RandomStream placement(seed, "maps/grove-alcove.v1");
        RandomStream loot(seed, "loot/grove-alcove.v1");
        for (size_t size = exterior.size(); size > 1; --size) std::swap(exterior[size - 1], exterior[placement.bounded(size)]);
        for (int cell : {center, exterior[0], exterior[1]}) {
            plan.loot.push_back({cell, loot.bounded(2) == 0 ? "vanilla.money" : "vanilla.stimpak"});
        }
        validateAlcove(plan);
        return plan;
    }
    throw std::invalid_argument("no connected single-opening alcove fits this template");
}

std::string toJson(const AlcovePlan& plan)
{
    validateAlcove(plan);
    std::ostringstream out;
    out << "{\"schema_version\":1,\"generator_version\":1,\"module\":\"grove-alcove.v1\",\"seed\":\""
        << plan.seed << "\",\"entry\":" << plan.input.entrance << ",\"walkable\":";
    array(out, plan.input.walkable);
    out << ",\"protected\":";
    array(out, plan.input.protectedCells);
    out << ",\"interior\":" << plan.interior << ",\"opening\":" << plan.opening << ",\"blockers\":";
    array(out, plan.blockers);
    out << ",\"loot\":[";
    for (size_t i = 0; i < plan.loot.size(); ++i) {
        if (i) out << ',';
        out << "{\"hex\":" << plan.loot[i].hex << ",\"prototype\":\"" << plan.loot[i].prototype << "\"}";
    }
    out << "]}\n";
    return out.str();
}
} // namespace wasteland
