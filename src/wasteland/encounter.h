// Wasteland Infinite additions. Asset-free alcove-guard.v1 placement contract.
#ifndef WASTELAND_ENCOUNTER_H
#define WASTELAND_ENCOUNTER_H
#include "alcove.h"

namespace wasteland {
struct EncounterPlan {
    uint64_t seed;
    LayoutInput input; // after alcove blockers; protects routes, opening and loot
    int enemyHex;
};
LayoutInput fixtureEncounter();
EncounterPlan generateEncounter(const AlcovePlan& alcove);
void validateEncounter(const EncounterPlan& plan);
void validateEncounter(const EncounterPlan& plan, const AlcovePlan& alcove);
std::string toJson(const EncounterPlan& plan);
} // namespace wasteland
#endif
