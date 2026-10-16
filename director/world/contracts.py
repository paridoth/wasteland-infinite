"""Strict v1 engine contract validation, with no third-party/runtime dependencies.

The local schema evaluator implements ONLY the keywords used by our shipped
schemas and rejects unknown ones. It is not a general JSON Schema implementation.
No remote references, coercions, format execution or model-generated code.
"""
from __future__ import annotations
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
MAX_BODY = 1_048_576


def _fail(message):
    raise ValueError(message)


def _pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            _fail(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def strict_json(text: str | bytes):
    if len(text if isinstance(text, bytes) else text.encode('utf-8')) > MAX_BODY:
        _fail('contract exceeds 1 MiB')
    return json.loads(text, object_pairs_hook=_pairs,
                      parse_constant=lambda value: _fail(f'nonfinite JSON: {value}'))


def validate_schema(value, schema, root=None):
    """Evaluate the closed, local schema subset used by the phase-1 contract."""
    root = schema if root is None else root
    supported = {'$schema', '$id', 'title', 'description', '$defs', '$ref',
                 'type', 'const', 'enum', 'minimum', 'maximum', 'minLength',
                 'maxLength', 'pattern', 'minItems', 'maxItems', 'uniqueItems',
                 'items', 'properties', 'required', 'additionalProperties'}
    if set(schema) - supported:
        _fail('schema uses an unsupported keyword')
    if '$ref' in schema:
        ref = schema['$ref']
        if not ref.startswith('#/$defs/') or ref.count('/') != 2:
            _fail('only local definition references are supported')
        validate_schema(value, root['$defs'][ref.split('/')[-1]], root)
        return
    types = {'object': dict, 'array': list, 'string': str, 'integer': int}
    if 'type' in schema and type(value) is not types[schema['type']]:
        _fail(f"expected {schema['type']}")
    if 'const' in schema and (type(value) is not type(schema['const']) or value != schema['const']):
        _fail('unsupported constant/version')
    if 'enum' in schema and not any(type(value) is type(v) and value == v for v in schema['enum']):
        _fail('value outside allowlist')
    if type(value) is int:
        if value < schema.get('minimum', value) or value > schema.get('maximum', value):
            _fail('integer outside range')
    if type(value) is str:
        if not schema.get('minLength', 0) <= len(value) <= schema.get('maxLength', len(value)):
            _fail('invalid string length')
        if 'pattern' in schema and re.fullmatch(schema['pattern'], value, flags=re.ASCII) is None:
            _fail('invalid string pattern')
    if type(value) is list:
        if not schema.get('minItems', 0) <= len(value) <= schema.get('maxItems', len(value)):
            _fail('invalid array size')
        if schema.get('uniqueItems') and len({json.dumps(v, sort_keys=True) for v in value}) != len(value):
            _fail('duplicate array element')
        if 'items' in schema:
            for item in value:
                validate_schema(item, schema['items'], root)
    if type(value) is dict:
        if set(schema.get('required', [])) - value.keys():
            _fail('missing fields')
        properties = schema.get('properties', {})
        if schema.get('additionalProperties') is False and value.keys() - properties.keys():
            _fail('unknown fields')
        for key, subschema in properties.items():
            if key in value:
                validate_schema(value[key], subschema, root)


def neighbors(cell):
    """Independent Python version of tileInit's verified even/odd offsets."""
    offsets = (-1, 199, 200, 201, 1, -200) if cell % 2 == 0 else (-201, -1, 200, 1, -199, -200)
    for offset in offsets:
        other = cell + offset
        if 0 <= other < 40000 and abs(other % 200 - cell % 200) <= 1:
            yield other


def validate_location(location):
    schema = json.loads((ROOT / 'schemas/location.v1.schema.json').read_text())
    validate_schema(location, schema)
    seed = int(location['seed'])
    if seed > 2**64 - 1:
        _fail('seed exceeds uint64')
    template = location['template']
    catalog = json.loads((ROOT / 'templates/catalog.v1.json').read_text())
    if not any(template['id'] == item['id'] and template['map'] == item['map'] for item in catalog['templates']):
        _fail('nonexistent template reference')
    expected_id = f"loc-{seed:016x}-{template['id']}"
    if location['id'] != expected_id:
        _fail('location identity mismatch')
    walkable = set(location['walkable'])
    blocked = set(location['obstacles'])
    reserved = set(location['reserved'])
    anchors = set(location['entrances'] + location['exits'])
    if walkable & blocked or not (anchors | reserved) <= walkable:
        _fail('invalid terrain references')
    seen = set(location['entrances'])
    pending = list(seen)
    while pending:
        for other in neighbors(pending.pop()):
            if other in walkable and other not in seen:
                seen.add(other)
                pending.append(other)
    if seen != walkable:
        _fail('disconnected terrain')
    occupied = reserved | anchors
    for index, spawn in enumerate(location['spawns']):
        if spawn['id'] != f'{expected_id}.loot-{index}':
            _fail('spawn identity mismatch')
        if spawn['prototype'] not in catalog['prototypes']:
            _fail('nonexistent prototype reference')
        if spawn['hex'] not in walkable or spawn['hex'] in occupied:
            _fail('spawn overlaps or is inaccessible')
        occupied.add(spawn['hex'])
    return location


def load_location(text):
    return validate_location(strict_json(text))
