"""Offline layout-diagnostic validation; no Director state or engine commands."""
import json
from pathlib import Path
from director.world.contracts import neighbors, strict_json, validate_schema

ROOT = Path(__file__).resolve().parents[2]


def connected(cells, entry):
    unseen = set(cells)
    if entry not in unseen:
        return False
    unseen.remove(entry)
    pending = [entry]
    while pending:
        for other in neighbors(pending.pop()):
            if other in unseen:
                unseen.remove(other)
                pending.append(other)
    return not unseen


def load_layout(raw):
    plan = strict_json(raw)
    validate_schema(plan, json.loads((ROOT / 'schemas/layout.v1.schema.json').read_text()))
    if int(plan['seed']) > 2**64 - 1:
        raise ValueError('layout seed exceeds uint64')
    walkable, protected, blocked = (set(plan[key]) for key in ('walkable', 'protected', 'blockers'))
    if plan['entry'] not in protected or not protected <= walkable or not blocked <= walkable or blocked & protected:
        raise ValueError('invalid layout cell references')
    if plan['blockers'][1] not in neighbors(plan['blockers'][0]):
        raise ValueError('partition blockers must be adjacent')
    if not connected(walkable, plan['entry']) or not connected(walkable - blocked, plan['entry']):
        raise ValueError('layout disconnects walkable cells')
    if len(walkable - blocked - protected) < 3:
        raise ValueError('layout leaves no loot capacity')
    return plan
