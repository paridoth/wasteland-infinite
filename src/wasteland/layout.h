// Wasteland Infinite additions. Asset-free bounded layout contract v1.
#ifndef WASTELAND_LAYOUT_H
#define WASTELAND_LAYOUT_H
#include "location.h"
#include <string>
#include <vector>

namespace wasteland {
struct LayoutInput {
    int entrance;
    std::vector<int> walkable;
    std::vector<int> protectedCells;
};
struct LayoutPlan {
    uint64_t seed;
    LayoutInput input;
    std::vector<int> blockers;
};
LayoutInput fixtureLayout();
LayoutPlan generateLayout(uint64_t seed, LayoutInput input);
void validateLayout(const LayoutPlan& plan);
std::string toJson(const LayoutPlan& plan);
bool visibleLayoutArt(int width, int height, int nontransparentPixels);
} // namespace wasteland
#endif
