// Wasteland Infinite additions. Runtime evidence: docs/wasteland/VALIDATION.md.
#include "encounter_runtime.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "art.h"
#include "combat_ai.h"
#include "critter.h"
#include "debug.h"
#include "location.h"
#include "map.h"
#include "object.h"
#include "party_member.h"
#include "proto.h"
#include "queue.h"
#include "scripts.h"
#include "stat.h"

namespace wasteland {
namespace {

bool validEncounterFlags(unsigned flags)
{
    using namespace fallout;
    return (flags & unsigned(OBJECT_HIDDEN | OBJECT_NO_BLOCK | OBJECT_MULTIHEX
        | OBJECT_NO_SAVE | OBJECT_NO_REMOVE)) == 0;
}

bool validPrototypeScript(int sid)
{
    using namespace fallout;
    return sid == -1 || (scriptTypeFromSid(sid) == SCRIPT_TYPE_CRITTER
        && scriptsIsValidScriptIndex(scriptIdFromSid(sid)));
}

bool partyPrototype(const fallout::ProtoId& protoId)
{
    const auto& partyIds = fallout::gPartyMemberProtoIds;
    return std::find(partyIds.begin(), partyIds.end(), protoId) != partyIds.end();
}

bool visibleIdleArt(fallout::Object* object)
{
    using namespace fallout;
    FrmImage image;
    if (FrmId(object).animationType() != ANIM_STAND
        || !image.lock(FrmId(object), 0, object->rotation)
        || image.getWidth() <= 0 || image.getHeight() <= 0
        || image.getWidth() > 256 || image.getHeight() > 256) return false;
    int pixels = image.getWidth() * image.getHeight();
    return std::count_if(image.getData(), image.getData() + pixels,
        [](unsigned char color) { return color != 0; }) >= 32;
}

bool hasQueuedEvents(fallout::Object* object)
{
    using namespace fallout;
    for (EventType type = EVENT_TYPE_FIRST; type < EVENT_TYPE_COUNT; type++) {
        if (queueHasEvent(object, type)) return true;
    }
    return (object->flags & OBJECT_QUEUED) != OBJECT_NONE;
}

// Native creation allocates script metadata without loading or executing its
// program (objectSetScriptFromProto -> scriptAdd -> _scr_find_str_run_info).
// Remove that fresh allocation before placement, hooks, refresh or rollback;
// objectDestroy would otherwise dispatch the prototype's destroy procedure.
bool detachFreshScript(fallout::Object* object, int prototypeSid, bool& metadataAbsent)
{
    using namespace fallout;
    if (object->sid == -1) {
        metadataAbsent = true;
        object->scriptIndex = -1;
        return prototypeSid == -1 && !hasQueuedEvents(object);
    }
    int allocatedSid = object->sid;
    Script* script = nullptr;
    bool fresh = prototypeSid != -1
        && scriptGetScript(allocatedSid, &script) == 0 && script != nullptr
        && scriptTypeFromSid(allocatedSid) == SCRIPT_TYPE_CRITTER
        && script->program == nullptr && script->flags == SCRIPT_FLAG_NONE
        && script->owner == object && script->ownerId == object->id
        && script->localVarsOffset == -1
        && script->index == scriptIdFromSid(prototypeSid)
        && object->scriptIndex == script->index
        && !hasQueuedEvents(object);
    // This API removes metadata and script timer events; it calls no script
    // procedure. Clear the object's references even if an invariant failed.
    int removed = scriptRemove(allocatedSid);
    object->sid = -1;
    object->scriptIndex = -1;
    Script* remaining = nullptr;
    metadataAbsent = scriptGetScript(allocatedSid, &remaining) != 0;
    debugPrint("WASTELAND ENCOUNTER SCRIPT: proto_sid=%d allocated_sid=%d fresh=%d removed=%d absent=%d queued=%d\n",
        prototypeSid, allocatedSid, fresh, removed, metadataAbsent, hasQueuedEvents(object));
    return fresh && removed == 0 && metadataAbsent && !hasQueuedEvents(object);
}

} // namespace

EncounterSource findEncounterSource(bool verbose)
{
    using namespace fallout;
    if (gDude == nullptr || !validHex(gDude->tile)) {
        throw std::runtime_error("encounter source requires a valid player");
    }
    // The starting temple contains no ant actor. Inspect the native registry,
    // without creating candidates or retaining cache pointers between loads.
    // proto_max_id is the exclusive upper index (the registry starts at one).
    int limit = proto_max_id(OBJ_TYPE_CRITTER);
    if (limit < 2 || limit > 4096) throw std::runtime_error("native critter registry exceeds encounter audit bounds");
    std::vector<EncounterSource> candidates;
    for (int index = 1; index < limit; ++index) {
        ProtoId requested(static_cast<CritterProtoTypeId>(index));
        if (requested == CritterProtoTypeId::Dude || partyPrototype(requested)) continue;
        Proto* proto = nullptr;
        if (protoGetProto(requested, &proto) != 0 || proto == nullptr || ProtoId(proto) != requested
            || proto->critter.data.killType != KILL_TYPE_GIANT_ANT) continue;
        // This stack value is only a read-only view for FrmImage/aiGetPacket.
        // It is never inserted, given an identity, or passed to creation APIs.
        Object inspection {};
        inspection.id = -1;
        inspection.cid = -1;
        inspection.sid = -1;
        inspection.scriptIndex = -1;
        inspection.tile = -1;
        inspection.pid = proto->pid;
        inspection.fid = proto->fid;
        inspection.rotation = FrmId(proto).rotation();
        if (!rotationIsValid(inspection.rotation)) inspection.rotation = ROTATION_NE;
        inspection.data.critter.combat.team = proto->critter.team;
        inspection.data.critter.combat.aiPacket = proto->critter.aiPacket;
        bool artVisible = visibleIdleArt(&inspection);
        AiPacket* packet = aiGetPacket(&inspection);
        bool validPacket = packet != nullptr && packet->packet_num == proto->critter.aiPacket;
        if (verbose) {
            debugPrint("WI ANT PROTOTYPE: index=%d pid=%d fid=%d rotation=%d flags=%u proto_sid=%d kill=%d team=%d ai=%d exact_packet=%d visible_idle=%d hp=%d ap=%d\n",
                index, proto->pid, proto->fid, inspection.rotation, unsigned(proto->flags),
                proto->sid, int(proto->critter.data.killType), proto->critter.team,
                proto->critter.aiPacket, validPacket, artVisible,
                critterGetStat(&inspection, STAT_MAXIMUM_HIT_POINTS),
                critterGetStat(&inspection, STAT_MAXIMUM_ACTION_POINTS));
        }
        if (!validEncounterFlags(unsigned(proto->flags))
            || (proto->critter.data.flags & CRITTER_INVULNERABLE) != CRITTER_NONE
            || !validPrototypeScript(proto->sid) || proto->critter.team < 0
            || proto->critter.team == gDude->data.critter.combat.team
            || !artVisible || !validPacket) continue;
        candidates.push_back({proto->pid, proto->fid, -1, inspection.rotation,
            proto->critter.team, proto->critter.aiPacket});
    }
    if (candidates.empty()) throw std::runtime_error("no verified native single-hex ant encounter source");
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) { return a.pid < b.pid; });
    const auto& source = candidates.front();
    debugPrint("WASTELAND ENCOUNTER SOURCE: registry=%d candidates=%d %s pid=%d fid=%d tile=%d rotation=%d team=%d ai=%d\n",
        limit - 1, int(candidates.size()),
        protoGetName(ProtoId(source.pid)), source.pid, source.fid, source.tile,
        source.rotation, source.team, source.aiPacket);
    return source;
}

fallout::Object* createEncounterAnt(const EncounterSource& source, int hex,
    std::vector<fallout::Object*>& created)
{
    using namespace fallout;
    Proto* proto = nullptr;
    if (gDude == nullptr || !validHex(hex) || !rotationIsValid(source.rotation)
        || ProtoId(source.pid).objectType() != OBJ_TYPE_CRITTER
        || protoGetProto(ProtoId(source.pid), &proto) != 0 || proto == nullptr
        || proto->critter.data.killType != KILL_TYPE_GIANT_ANT
        || partyPrototype(ProtoId(source.pid)) || source.tile != -1
        || proto->fid != source.fid || !validEncounterFlags(unsigned(proto->flags))
        || (proto->critter.data.flags & CRITTER_INVULNERABLE) != CRITTER_NONE
        || !validPrototypeScript(proto->sid) || source.team < 0
        || source.team == gDude->data.critter.combat.team
        || source.team != proto->critter.team || source.aiPacket != proto->critter.aiPacket
        || objectFindFirstAtLocation(gDude->elevation, hex) != nullptr
        || _obj_blocking_at(gDude, hex, gDude->elevation) != nullptr) {
        throw std::runtime_error("encounter source or destination changed after audit");
    }

    // _proto_update_init recomputes shared derived stats during native critter
    // creation. Restore the complete original data immediately, without a yield
    // or script dispatch, so creating an ant cannot rebalance existing ants.
    const CritterProtoData originalData = proto->critter.data;
    int prototypeSid = proto->sid;
    int mapLocalVarsBefore = gMapLocalVarsLength;
    Object* object = nullptr;
    int result = objectCreateWithProtoId(&object, ProtoId(source.pid));
    bool statsChanged = std::memcmp(&originalData, &proto->critter.data, sizeof(originalData)) != 0;
    proto->critter.data = originalData;
    // Nothing which can throw or destroy the object precedes script removal.
    bool metadataAbsent = false;
    bool detached = object == nullptr || detachFreshScript(object, prototypeSid, metadataAbsent);
    if (object == nullptr) throw std::runtime_error("native ant creation failed");
    if (!detached) {
        // The caller immediately unloads/resets a failed proof map. If native
        // script removal became inconsistent, retain its owner until that reset
        // rather than leave Script.owner pointing at freed object memory.
        queueRemoveEvents(object);
        if (metadataAbsent) objectDestroy(object, nullptr);
        throw std::runtime_error("fresh ant script detachment failed; proof map reset required");
    }
    if (gMapLocalVarsLength != mapLocalVarsBefore) {
        objectDestroy(object, nullptr); // already detached; cannot execute a script
        throw std::runtime_error("fresh ant changed native map locals; proof map reset required");
    }
    // Parent rollback receives only an object with no script or queued event.
    created.push_back(object);
    if (result != 0 || object->sid != -1 || object->scriptIndex != -1
        || object->fid != source.fid || object->data.inventory.length != 0
        || !validEncounterFlags(unsigned(object->flags))
        || objectIsPartyMember(object) || partyPrototype(ProtoId(object))
        || !critterIsActive(object) || hasQueuedEvents(object)) {
        throw std::runtime_error("fresh ant is not a safe native encounter object");
    }
    // Registry metadata uses the prototype defaults. Require native creation to
    // preserve them instead of invoking setters which can alter party protos.
    AiPacket* packet = aiGetPacket(object);
    int maximumHp = critterGetStat(object, STAT_MAXIMUM_HIT_POINTS);
    int maximumAp = critterGetStat(object, STAT_MAXIMUM_ACTION_POINTS);
    if (packet == nullptr || packet->packet_num != source.aiPacket
        || object->data.critter.combat.team != source.team
        || object->data.critter.combat.aiPacket != source.aiPacket
        || object->data.critter.hp <= 0 || object->data.critter.hp != maximumHp
        || object->data.critter.combat.ap <= 0 || object->data.critter.combat.ap != maximumAp
        || objectSetLocation(object, hex, gDude->elevation, nullptr) != 0
        || objectSetRotation(object, source.rotation, nullptr) != 0
        || _obj_blocking_at(gDude, hex, gDude->elevation) != object
        || !visibleIdleArt(object) || hasQueuedEvents(object)) {
        throw std::runtime_error("native ant placement or initialized stats failed verification");
    }
    debugPrint("WASTELAND ENCOUNTER: created pid=%d fid=%d tile=%d rotation=%d team=%d ai=%d hp=%d ap=%d sid=%d script_index=%d restored_proto_stats=%d map_locals=%d/%d\n",
        object->pid, object->fid, object->tile, object->rotation,
        object->data.critter.combat.team, object->data.critter.combat.aiPacket,
        object->data.critter.hp, object->data.critter.combat.ap, object->sid,
        object->scriptIndex, statsChanged, mapLocalVarsBefore, gMapLocalVarsLength);
    return object;
}

} // namespace wasteland
