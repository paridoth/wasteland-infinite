// Wasteland Infinite additions. Independent grove-alcove.v1 contract.
#ifndef WASTELAND_ALCOVE_H
#define WASTELAND_ALCOVE_H
#include "layout.h"

namespace wasteland {
struct AlcoveLoot {
    int hex;
    std::string prototype;
};
struct AlcovePlan {
    uint64_t seed;
    LayoutInput input;
    int interior;
    int opening;
    std::vector<int> blockers;
    std::vector<AlcoveLoot> loot;
};
AlcovePlan generateAlcove(uint64_t seed, LayoutInput input);
void validateAlcove(const AlcovePlan& plan);
std::string toJson(const AlcovePlan& plan);
} // namespace wasteland
#endif
