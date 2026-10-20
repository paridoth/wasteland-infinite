// Wasteland Infinite additions. Native encounter placement stays scriptless.
#ifndef WASTELAND_ENCOUNTER_RUNTIME_H
#define WASTELAND_ENCOUNTER_RUNTIME_H

#include <vector>

#include "obj_types.h"

namespace wasteland {

struct EncounterSource {
    int pid;
    int fid;
    int tile; // -1: metadata audited from the native prototype registry
    fallout::Rotation rotation;
    int team;
    int aiPacket;
};

EncounterSource findEncounterSource(bool verbose);
fallout::Object* createEncounterAnt(const EncounterSource& source, int hex,
    std::vector<fallout::Object*>& created);

} // namespace wasteland

#endif
