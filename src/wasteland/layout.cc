// Wasteland Infinite additions. Two adjacent blockers, independent RNG stream.
#include "layout.h"
#include <algorithm>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>

namespace wasteland {
bool visibleLayoutArt(int width, int height, int nontransparentPixels)
{
    return width > 0 && height > 0 && width <= 256 && height <= 256
        && nontransparentPixels >= 32 && nontransparentPixels <= width * height;
}
namespace {
std::set<int> cells(const std::vector<int>& input)
{
    std::set<int> result;
    for (int cell : input) {
        if (!validHex(cell) || !result.insert(cell).second) throw std::invalid_argument("invalid or duplicate layout cell");
    }
    return result;
}

bool connected(std::set<int> walkable, int entrance)
{
    if (!walkable.erase(entrance)) return false;
    std::queue<int> pending;
    pending.push(entrance);
    while (!pending.empty()) {
        int cell = pending.front();
        pending.pop();
        for (int dir = 0; dir < 6; ++dir) {
            int next = neighbor(cell, dir);
            if (walkable.erase(next)) pending.push(next);
        }
    }
    return walkable.empty();
}

void validateInput(const LayoutInput& input)
{
    if (input.walkable.size() > 61 || input.walkable.size() < 6 || input.protectedCells.size() > 61) {
        throw std::invalid_argument("layout must contain 6..61 cells");
    }
    auto walkable = cells(input.walkable);
    auto protectedCells = cells(input.protectedCells);
    if (!protectedCells.count(input.entrance)
        || !std::includes(walkable.begin(), walkable.end(), protectedCells.begin(), protectedCells.end())
        || !connected(walkable, input.entrance)) throw std::invalid_argument("invalid layout protection or connectivity");
}

void array(std::ostringstream& out, const std::vector<int>& values)
{
    out << '[';
    for (size_t index = 0; index < values.size(); ++index) {
        if (index) out << ',';
        out << values[index];
    }
    out << ']';
}
} // namespace

LayoutInput fixtureLayout()
{
    LayoutInput input { 20100, {}, { 20100, 20099, 19898 } };
    std::set<int> seen { input.entrance };
    std::queue<std::pair<int, int>> pending;
    pending.push({input.entrance, 0});
    while (!pending.empty()) {
        auto [cell, depth] = pending.front();
        pending.pop();
        if (depth == 3) continue;
        for (int dir = 0; dir < 6; ++dir) {
            int next = neighbor(cell, dir);
            if (next >= 0 && seen.insert(next).second) pending.push({next, depth + 1});
        }
    }
    input.walkable.assign(seen.begin(), seen.end());
    return input;
}

void validateLayout(const LayoutPlan& plan)
{
    validateInput(plan.input);
    if (plan.blockers.size() != 2) throw std::invalid_argument("layout requires two blockers");
    auto blocked = cells(plan.blockers);
    auto remaining = cells(plan.input.walkable);
    auto protectedCells = cells(plan.input.protectedCells);
    bool adjacent = false;
    for (int dir = 0; dir < 6; ++dir) adjacent |= neighbor(plan.blockers[0], dir) == plan.blockers[1];
    for (int cell : blocked) {
        if (!remaining.erase(cell) || protectedCells.count(cell)) throw std::invalid_argument("layout blocks a protected or occupied cell");
    }
    if (!adjacent || !connected(remaining, plan.input.entrance)) throw std::invalid_argument("layout disconnects walkable cells");
    size_t available = 0;
    for (int cell : remaining) if (!protectedCells.count(cell)) ++available;
    if (available < 3) throw std::invalid_argument("layout leaves no loot capacity");
}

LayoutPlan generateLayout(uint64_t seed, LayoutInput input)
{
    validateInput(input);
    std::sort(input.walkable.begin(), input.walkable.end());
    std::sort(input.protectedCells.begin(), input.protectedCells.end());
    std::vector<std::pair<int, int>> candidates;
    for (int cell : input.walkable) {
        if (std::binary_search(input.protectedCells.begin(), input.protectedCells.end(), cell)) continue;
        for (int dir = 0; dir < 6; ++dir) {
            int next = neighbor(cell, dir);
            if (next > cell && std::binary_search(input.walkable.begin(), input.walkable.end(), next)
                && !std::binary_search(input.protectedCells.begin(), input.protectedCells.end(), next)) {
                candidates.push_back({cell, next});
            }
        }
    }
    std::sort(candidates.begin(), candidates.end());
    RandomStream stream(seed, "layouts/entry-partition.v1");
    for (size_t size = candidates.size(); size > 1; --size) {
        std::swap(candidates[size - 1], candidates[stream.bounded(size)]);
    }
    for (auto [first, second] : candidates) {
        LayoutPlan plan { seed, input, {first, second} };
        try { validateLayout(plan); return plan; } catch (const std::invalid_argument&) { }
    }
    throw std::invalid_argument("no connected two-object partition fits this template");
}

std::string toJson(const LayoutPlan& plan)
{
    validateLayout(plan);
    std::ostringstream out;
    out << "{\"schema_version\":1,\"generator_version\":1,\"layout\":\"entry-partition.v1\",\"seed\":\""
        << plan.seed << "\",\"entry\":" << plan.input.entrance << ",\"walkable\":";
    array(out, plan.input.walkable);
    out << ",\"protected\":";
    array(out, plan.input.protectedCells);
    out << ",\"blockers\":";
    array(out, plan.blockers);
    out << "}\n";
    return out.str();
}
} // namespace wasteland
