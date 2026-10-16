# Wasteland Infinite roadmap

## Phase 0 — foundation

Done: clean upstream SHA/tag, unmodified build attempt and isolated type fix,
working x86-64 baseline, license preservation, asset search, modular opt-in engine
subsystem, offline build/tests and source integration audit. Existing checkout
already occupies WastelandInfinite/engine; src stays in place, with director/,
schemas/, templates/, tests/, tools/, docs/ and assets/ inside engine/.

Owned game assets were not found. No native gameplay or graphical compatibility
has been validated. This is an explicit asset-dependent gate, not a test success.

## Phase 1 — first procedural proof

Done: deterministic template representation, named uint64 RNG streams, three
allowlisted loot references, connectivity/overlap checks, schema v1, seed fixtures,
asset-free inspector in the real engine executable, opt-in new-game placement
adapter, safe staging tool and rollback of partial object placement.

Next task: stage owned vanilla assets, launch --wi-dev-seed=42, select NEW GAME,
inspect all three visible/pickable objects and compare the runtime JSON to repeated
fresh runs. Verify player routes, actual map exit, combat, animation, inventory,
save/reload and a second new game. Run the same smoke test with mode disabled.
Record actual template geometry and asset versions. Mark the template validated
only after this succeeds. A bounded loot overlay is not a new dungeon/world.

Then add a dedicated known-good copied map template with verified exit grids and
entry/exit positions. Keep inherited tiles until tested tile art indices are
available. Expand modular rooms and deterministic enemy/loot placement. Fully
generated MAP files require round-trip tests against CE/mapper readers/writers.

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
| Deterministic seed replay | C++ replay/order tests; five pinned output fixtures; real engine CLI |
| Serialization/schema compatibility | Python consumes C++ JSON, round-trip, closed schema/version/type tests |
| Object/tile references | Symbol allowlist and catalog checks; inherited tile policy; runtime proto/art checks compiled, execution pending |
| Reachable entrances/exits | Synthetic full connectivity plus engine pathfinder calls; actual map exits pending |
| No mandatory overlaps | C++ and Python reject overlap, reservations and entrance/exit collisions |
| Nonexistent entity references | Template, prototype, location and placement identity rejection; full simulation entities deferred |
| Duplicate event rejection | Deferred with event journal; duplicate JSON keys already rejected |
| Quest graph validity | Deferred with quest system |
| Native save/checkpoint consistency | Source audit/strategy and staging isolation tested; hooks/checkpoints deferred |
| Unavailable LLM provider | Deferred with provider adapters; current tests/network usage have no LLM dependency |

Do not mark deferred tests passed or claim the playable prototype succeeded from
fixture/compile results. No original installation or saves are used for testing.
