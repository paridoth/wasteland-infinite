# Wasteland Infinite architecture

This is a modified Fallout 2 CE engine, retaining its rendering, SPECIAL, combat,
inventory, movement, animation and UI implementations. See LICENSE.md for the
upstream Sustainable Use License and ENGINE_INTEGRATION.md for verified interfaces.
This milestone does not replace those systems or add real-time combat.

## Running code in this milestone

- `src/wasteland/location.*`: standalone C++17 deterministic generator and domain
  validation. Synthetic courtyard template, three loot placements, connectivity
  checks, JSON output. No engine pointers, network access, filesystem reads or
  Fallout random-number state. Template ordering is canonicalized.
- `src/wasteland/layout.*`: independent `entry-partition.v1` planner with a named
  `layouts/entry-partition.v1` stream, 6..61 local cells and two adjacent blockers.
  Entrance/native route reservations cannot be occupied; all remaining cells
  must stay connected with at least three free loot cells. Its separate versioned
  schema and five synthetic seed fixtures leave location.v1 output unchanged.
- `src/wasteland/options.*`: strict uint64 options, disabled default, runtime
  directory/config guard. `runtime.*` bridges the generator to verified CE APIs.
- `src/wasteland/alcove.*`: independent `grove-alcove.v1` module, with 10..127
  audited cells, five blockers enclosing one interior cell, one opening, one
  interior reward and two exterior loot objects. Named streams
  `rooms/grove-alcove.v1`, `maps/grove-alcove.v1` and `loot/grove-alcove.v1` separate
  geometry, exterior placement and item selection. The closed alcove schema and
  five fixtures are independent of location.v1 and partition.v1. Diagnostic loot
  cells are local module data, without campaign/entity persistence bindings.
- `schemas/location.v1.schema.json`: closed schema with explicit schema and
  generator versions. Decimal string seed preserves all 64 bits through JSON.
  `templates/catalog.v1.json` maps template/prototype symbols to audited engine
  names. Fixture geometry is not a distributable engine map.
- `director/world/contracts.py`: offline Python consumer, rejecting duplicate
  keys, nonfinite values, oversized bodies, extra fields, unsupported versions,
  unknown references, unreachable geometry and overlapping mandatory spawns.
  Its dependency-free evaluator supports only the shipped schema's keywords and
  rejects unsupported keywords; it is not a general JSON Schema library.
- `tools/wasteland/stage_runtime.py`: read-only installation access and actual
  copies into ignored runtime/. Does not reuse original configuration or saves.
- `tools/wasteland/stage_template.py`: pinned archive/map SHA-256 provenance,
  native DAT extraction of unchanged ARTEMPLE/ARCAVES copies, separate references
  and startup marker. `--wi-template-seed` checks equality with those references
  and audits actual native exit grids before placement. The independent v1 audit
  schema records native destinations and current entry path lengths; it neither
  replaces location.v1 nor binds Director/save state. Zero path length means no
  path found (or the exit is the entry cell), and -1 means the path was not tested.
- `--wi-layout-seed` adds the partition only to the copied new-game template. The
  runtime reconstructs/reserves existing native exit paths and verifies all local
  movement and previously reachable exits after placing props. It resolves its
  prop from the pinned map's existing scriptless generic scenery/walls, requiring
  matching loaded prototype/art, no inventory, no hidden/nonblocking/multihex
  flags, native save/removal capability, and art at most 256x256 with at least 32
  nontransparent pixels. Automap marker prototypes are explicitly excluded.
  No external prototype ID is accepted. Objects
  use native creation, location/rotation, destruction and save serialization.
- `--wi-probe-layout` initializes Narg and the same native new-game path, returns
  placement status as an exit code, and cleans up without running the input loop.
  This owned-asset integration probe is separate from asset-free CTest and visual
  gameplay validation. Interactive layout failure returns to the menu explicitly.
- `--wi-alcove-seed` uses the same copied-template guards, native prop audit,
  rollback, local movement/exit postchecks and new-game-only hook. Only this mode
  uses occupancy sampling to radius six (at most 127 hexes); earlier modes keep
  radius four. The opening and protected routes receive no loot. Its independent
  `wasteland-alcove.json` replaces the location diagnostic for this mode, leaving
  the location generator unchanged. `--wi-probe-alcove` uses the same bounded
  Narg probe lifecycle. Inherited floor/roof and native transitions remain intact.
- `src/wasteland/encounter.*`: independent `alcove-guard.v1` planner and
  `encounters/alcove-guard.v1` RNG stream. It adds one ant cell at least four
  physical hexes from entry, protects the alcove opening/loot/native routes,
  and keeps all other sampled cells connected while the ant blocks its cell.
  Its synthetic inspector uses a separate radius-four fixture; prior fixtures
  retain their geometry and bytes. The encounter diagnostic can be validated
  against the sibling alcove diagnostic for exact geometry/protection agreement.
- `encounter_runtime.*` resolves the symbolic ant through the native prototype
  registry, using bounded semantic/art/team/AI checks. Native creation initializes
  HP/AP; the adapter restores shared derived prototype stats and removes freshly
  allocated, unexecuted script metadata before placement. Existing script/map
  local state stays intact. The generated ant uses native combat AI after the
  player initiates combat; this proof adds no autonomous hostility script.

## Determinism contract v1

SplitMix64 with unsigned 64-bit wrapping, initialized with campaign seed XOR
FNV-1a-64 of the stream name. Bounded draws use rejection sampling, never
implementation-dependent C++ distributions. Independent names are `topology`,
`maps/<template-id>`, `encounters` and `loot/<template-id>`; topology and encounters
are reserved for subsequent generation. Map-stream Fisher-Yates chooses spawn
cells; loot-stream draws choose allowed items. Changing loot draws cannot consume
the map stream. Replay fixtures pin serialized output for 0, 1, 42,
9007199254740993 and 18446744073709551615. Algorithm changes need a generator
version change and reviewed new fixtures.

Location IDs derive from seed/template for this single-location proof. Object
IDs derive from location ID/placement index. These are stable domain references,
not native object IDs. They must be scoped by immutable campaign UUID when
multi-location persistence arrives. Seed alone must never identify a campaign.

The v1 location contains biome/type, inherited floor/roof policy, walkable cells,
obstacles, reserved cells, entrance/exit anchors, spawns and empty layout/NPC/
script-anchor lists. The engine template's exit is a return-to-start anchor;
it does NOT create an exit grid. New buildings, rooms, doors, real transitions,
critter spawns and scripts need new verified contracts/templates.

The inspector emits synthetic `fixture.courtyard.v1`. Runtime builds a different
`engine.artemple.v1` template from actual local engine occupancy/pathfinding.
They are deliberately different inputs to the SAME generator. Runtime replay
requires identical template geometry and state, not only the same seed. There
is no claim that a synthetic fixture is a faithful extraction of ARTEMPLE.MAP.

## Planned boundaries — not implemented yet

A local Python HTTP service will own generation requests and world simulation.
Bind to 127.0.0.1; use strict versioned contracts, a bounded request queue and
short worker timeouts. The C++ engine must never wait on HTTP in rendering,
movement, animation, combat or modal dialogue. Apply validated responses on the
engine thread only. The service is not started by this milestone.

Provider protocol: deterministic mock first, then environment-configured local
and hosted OpenAI-compatible adapters. Each needs bounded input/output tokens,
short request timeouts, controlled retries, request/concurrency/spend budgets,
optional version-keyed cache, diagnostics and deterministic fallback. No keys
or endpoints are read from .env yet; .env.example is a future configuration
contract. The engine currently has NO networking or LLM dependency.

SQLite design: campaigns, locations, characters, factions, relationships,
knowledge/beliefs, quests, objective world facts, inventory references, append-only
game events and immutable save checkpoints. Stable UUIDs are independent of CE
object IDs. Separate objective truth from NPC personality, goals, memory, beliefs
and testimony. Deterministic reducers own mutations, with transactional duplicate
event detection and reproducible projections. Older checkpoints restore their
branch, never retain future facts. No database has been created in this milestone.

Whitelisted actions must resolve entities and check preconditions and consequences
before journaling. Models propose dialogue plus actions, never SQL, code or engine
scripts. Begin with finite quest templates and graph validation: real referenced
entities/rewards, reachable goals, acyclic mandatory prerequisites and meaningful
failure/multiple outcomes. An isolated dialogue proof precedes any classic
dialogue replacement. Detailed checkpoint/native-slot binding is in the audit.
