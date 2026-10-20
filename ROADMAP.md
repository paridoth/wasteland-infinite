# Wasteland Infinite roadmap

## Phase 0 — foundation

Done: clean upstream SHA/tag, unmodified build attempt and isolated type fix,
working x86-64 baseline, license preservation, asset search, modular opt-in engine
subsystem, offline build/tests and source integration audit. Existing checkout
already occupies WastelandInfinite/engine; src stays in place, with director/,
schemas/, templates/, tests/, tools/, docs/ and assets/ inside engine/.

Owned Steam game assets have been staged into an isolated runtime on Josh.
Three-object visibility/pickup, movement, inventory, combat, temple exits and
native save/reload are confirmed by the user. A fresh seed-42 run reproduced the
runtime diagnostic byte-for-byte; the user also confirmed the disabled-build
gameplay, music and save/reload comparison passed.

## Phase 1 — first procedural proof

Done: deterministic template representation, named uint64 RNG streams, three
allowlisted loot references, connectivity/overlap checks, schema v1, seed fixtures,
asset-free inspector in the real engine executable, opt-in new-game placement
adapter, safe staging tool and rollback of partial object placement.

Initial runtime placement succeeded on ARTEMPLE.MAP with --wi-dev-seed=42; the user
confirmed all three objects visible/pickable, movement, inventory, combat, actual
temple exits and save/reload without duplicate loot. A fresh run with the same
Narg preset produced byte-identical runtime JSON. The user also confirmed music,
normal gameplay/save behavior and absence of WI loot with the feature compiled out.
The bounded seed-42 ARTEMPLE proof is validated for the recorded Steam archives,
geometry and Narg preset; see docs/wasteland/VALIDATION.md for evidence and scope.
A bounded loot overlay is not a new dungeon/world. The Director remains unchanged.

Completed copied-template proof: pinned owned ARTEMPLE/ARCAVES copies in a separate runtime, with native
exit-grid/entry audit and a user-confirmed temple/cave round trip, music and
save/reload without duplicate loot. Eighteen temple exit grids resolve to native
maps; three cave exits are currently reachable from entry. The fifteen village
grids have no path at initial audit time and remain outside this validation.
See templates/artemple-copy.v1.json and docs/wasteland/VALIDATION.md for scope.

Completed first bounded layout change: a two-tree obstacle cluster on the copied
template, with a separate versioned planner/fixtures, protected native cave routes,
visible scriptless source selection and rollback/path postchecks. Two native
probes reproduced layout/loot/source metadata; the user confirmed both trees,
movement/loot access, cave round trip and save/reload without duplicates.
Implemented next bounded module: `grove-alcove.v1`, five trees enclosing one
interior loot cell with one opening and two exterior loot objects. It has its
own contract, named streams and fixtures; location.v1 and partition.v1 replay
remain unchanged. Two fresh native seed-42/Narg probes pass with identical output
and all initially reachable cave exits preserved. The user confirmed the alcove
visibility/pickup, cave round trip and save/reload checks passed; logs corroborate
the transitions and all 27 save/load handlers. This bounded module is validated
for seed 42, Narg and the pinned assets.
Implemented next bounded enemy step: `alcove-guard.v1` composes the same alcove
with one deterministic ant, using a separate planner/schema/fixtures and RNG
stream. Two native probes match and verify health/AP, native team/AI, removal of
fresh unexecuted script metadata, unchanged map locals and all initial cave paths.
The user confirmed combat, live/dead save persistence and cave round trip. Logs
corroborate native ant AI activity, two complete save/load cycles across sessions,
return travel, one placement per new game and normal exits. This remains scoped
to seed 42, Narg and the pinned Steam assets.
Retain inherited tiles until tested tile art indices are available. Expand modular rooms
and deterministic enemy/loot placement incrementally. Fully
generated MAP files require round-trip tests against CE/mapper readers/writers.

Paused by user request after this encounter proof. Before pausing, WI test modes
were changed to skip the new-game Elder movie and staging now defaults to running.
The current isolated encounter runtime also uses running; original configuration
and Director functionality are unchanged.

## Subsequent milestones — intentionally not delivered together

1. Offline SQLite journal/reducer, stable entity bindings, transactional duplicate
   rejection and old-save branch restoration; synthetic save/checkpoint tests.
2. Loopback service, strict action API and mock provider; timeout/budget/cache
   tests before local/hosted OpenAI-compatible adapters. No engine loop blocking.
3. Small quest graphs with prerequisites, entity/reward validation and multiple
   solutions; prevent cycles, missing targets and unwinnable dependencies.
4. Separate dialogue proof, persistent NPC knowledge/beliefs and personalities.
5. Faction simulation, diplomacy and cascading consequences, deterministic first.
6. World-scale generation and campaign regression/recovery hardening.

## Validation coverage

| Requirement | Current evidence |
| --- | --- |
| Deterministic seed replay | C++ replay/order tests; five pinned output fixtures; real engine CLI; two fresh seed-42 runtime diagnostics are byte-identical |
| Serialization/schema compatibility | Python consumes C++ JSON, round-trip, closed schema/version/type tests |
| Object/tile references | Symbol allowlist and catalog checks; inherited tile policy; runtime proto/art checks executed; user confirmed all three objects visible/pickable |
| Reachable entrances/exits | Synthetic full connectivity plus engine pathfinder calls; user confirmed movement and actual temple exits |
| No mandatory overlaps | C++ and Python reject overlap, reservations and entrance/exit collisions |
| Nonexistent entity references | Template, prototype, location and placement identity rejection; full simulation entities deferred |
| Duplicate event rejection | Deferred with event journal; duplicate JSON keys already rejected |
| Quest graph validity | Deferred with quest system |
| Native save/checkpoint consistency | User confirmed native save/reload without duplicate loot; all 27 native load handlers completed; Director save hooks/checkpoints deferred |
| Unavailable LLM provider | Deferred with provider adapters; current tests/network usage have no LLM dependency |

Do not mark deferred tests passed or claim the playable prototype succeeded from
fixture/compile results. No original installation or saves are used for testing.
