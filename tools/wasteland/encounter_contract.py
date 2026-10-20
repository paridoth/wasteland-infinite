"""Offline alcove-guard validation; no Director state or engine commands."""
import json
from pathlib import Path

from director.world.contracts import neighbors, strict_json, validate_schema
from tools.wasteland.alcove_contract import load_alcove
from tools.wasteland.layout_contract import connected

ROOT = Path(__file__).resolve().parents[2]


def load_encounter(raw, alcove_raw=None):
    plan = strict_json(raw)
    validate_schema(plan, json.loads((ROOT / 'schemas/encounter.v1.schema.json').read_text()))
    if int(plan['seed']) > 2**64 - 1:
        raise ValueError('encounter seed exceeds uint64')
    walkable, protected = (set(plan[key]) for key in ('walkable', 'protected'))
    entry, enemy = plan['entry'], plan['enemy']['hex']
    if entry not in protected or not protected <= walkable or enemy not in walkable or enemy in protected:
        raise ValueError('invalid encounter cell references')
    if not connected(walkable, entry) or not connected(walkable - {enemy}, entry):
        raise ValueError('encounter disconnects walkable cells')

    # Expand the physical grid, independent of local obstacles and path detours.
    # A long walk around an obstacle cannot make a nearby spawn safe at entry.
    near_entry, frontier = {entry}, {entry}
    for _ in range(plan['entry_clearance'] - 1):
        frontier = {other for cell in frontier for other in neighbors(cell)} - near_entry
        near_entry |= frontier
    if enemy in near_entry:
        raise ValueError('encounter violates entry clearance')

    if alcove_raw is not None:
        alcove = load_alcove(alcove_raw)
        expected_protection = (set(alcove['protected']) | {alcove['opening']}
                               | {item['hex'] for item in alcove['loot']})
        if (plan['seed'] != alcove['seed'] or entry != alcove['entry']
                or walkable != set(alcove['walkable']) - set(alcove['blockers'])
                or protected != expected_protection):
            raise ValueError('encounter does not match alcove geometry and protection')
    return plan
