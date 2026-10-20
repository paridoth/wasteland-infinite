"""Offline grove-alcove validation; no Director state or engine commands."""
import json
from pathlib import Path

from director.world.contracts import neighbors, strict_json, validate_schema
from tools.wasteland.layout_contract import connected

ROOT = Path(__file__).resolve().parents[2]


def load_alcove(raw):
    plan = strict_json(raw)
    validate_schema(plan, json.loads((ROOT / 'schemas/alcove.v1.schema.json').read_text()))
    if int(plan['seed']) > 2**64 - 1:
        raise ValueError('alcove seed exceeds uint64')
    walkable, protected, blocked = (set(plan[key]) for key in ('walkable', 'protected', 'blockers'))
    interior, opening, entry = (plan[key] for key in ('interior', 'opening', 'entry'))
    if (entry not in protected or not protected <= walkable or not blocked <= walkable
            or blocked & protected or interior not in walkable or opening not in walkable
            or interior in protected or interior == opening or opening in blocked):
        raise ValueError('invalid alcove cell references')
    ring = set(neighbors(interior))
    if len(ring) != 6 or blocked | {opening} != ring:
        raise ValueError('alcove requires five neighboring blockers and one opening')
    remaining = walkable - blocked
    if not connected(walkable, entry) or not connected(remaining, entry):
        raise ValueError('alcove disconnects walkable cells')
    if not (set(neighbors(opening)) & remaining - {interior}):
        raise ValueError('alcove opening has no outside approach')
    outside = remaining - protected - {interior, opening}
    if len(outside) < 2:
        raise ValueError('alcove leaves no outside loot capacity')
    positions = [item['hex'] for item in plan['loot']]
    if (positions[0] != interior or len(set(positions)) != 3
            or not set(positions[1:]) <= outside):
        raise ValueError('invalid alcove loot placement')
    return plan
