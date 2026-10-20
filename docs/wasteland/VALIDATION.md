# Wasteland Infinite session validation — 2026-10-16

## Testing defaults and requested pause — 2026-10-18

After completing the ant follow-up, the user requested a pause after removing
the new-game intro and defaulting to running for future testing. Opt-in WI modes
now skip the Elder movie when starting NEW GAME. Feature-disabled/default native
gameplay retains its movie behavior. Future staged runtimes include
`[preferences] running=1`; the current isolated encounter runtime also has that
setting. Native Shift-to-walk behavior is retained, and existing saves keep their
native saved preferences. Original installation configuration was not edited.

Both feature-ON/OFF builds pass, 2/2 CTest suites pass (23 Python methods), and
`git diff --check` passes. The movie branch and native running-setting consumer
were checked in source; no additional interactive session was launched after the
pause request. All 87 original files still match their recorded hashes, and the
Director remains unchanged. Work is paused at the user's request.

## Seeded ant encounter — 2026-10-18

Added independent `alcove-guard.v1` core planner, closed encounter schema,
standalone/paired offline validator, five new seed fixtures and opt-in
`--wi-inspect-encounter`, `--wi-encounter-seed` and `--wi-probe-encounter` modes.
It composes the existing alcove without changing its generator or loot positions.
The encounter reserves all native routes, the alcove opening and all three loot
cells, excludes physical hex radius three around entry, and keeps every remaining
local cell connected while the ant blocks its cell. Runtime repeats native
movement/exit checks after enemy placement. The Director remains unchanged.

Feature-ON build and 2/2 CTest suites PASS, including 23 Python methods. All five
original location, five partition and five alcove fixtures remain unchanged.
New tests cover replay and caller ordering, a dedicated radius-four fixture,
physical entry clearance versus long path detours, narrow corridors, exact paired
alcove geometry/protection, loot/opening collisions, closed schema/types/versions,
uint64 boundaries and runtime guards. Synthetic seed-42 encounter SHA-256:
`2ab7060a413154ef9d0e21aea7bb1d46a834cad5764bb8d8e85b09d791fd18e6`.
The feature-OFF build also passes.

The first source audit correctly exited two: the outdoor ARTEMPLE map has only
Narg and Cameron, with no ant actor to duplicate. Preserved that failed audit in
ignored `runtime/proof-encounter-probe/evidence-no-map-ant/`. Source discovery now
scans the native critter registry (capped at 4096 entries), selects the lowest
valid giant-ant PID, and creates no candidate objects. It requires non-party
identity, giant-ant kill type, single-hex/saveable flags, visible idle art,
hostile native team and exact AI-packet lookup. Only numeric audited metadata
crosses registry cache loads; no prototype pointer is retained across them.
The plan contains only `template.hostile-ant`, never a raw PID or script.

Native creation initializes HP/AP and temporarily recalculates shared prototype
stats; the adapter immediately restores `CritterProtoData`. Prototype script
creation only allocates metadata; source audit traced `objectSetScriptFromProto`
through `scriptAdd`/`_scr_find_str_run_info`, with no script procedure dispatch.
Before placement or rollback the adapter checks null program/default flags,
owner/index, unallocated locals and no events, removes that new script, and clears
`sid`/`scriptIndex`. Map-local count must remain unchanged. Existing actors and
shared prototype script/team/AI fields are not edited. Native saved-object loading
preserves a missing script rather than automatically attaching the prototype
script. Combat is deliberately player-initiated; native AI handles subsequent
turns. No custom hostility/quest script or Director behavior was added.

Two copied-runtime native probes completed with exit zero in
`runtime/proof-encounter-probe` and `runtime/proof-encounter-play`. The latter
has its own fresh archives/maps and had no save slot before the interactive test.
All encounter, alcove and exit-audit diagnostics match byte-for-byte. Preserved
probe evidence in each runtime's `evidence-probe/` directory:

- Registry audit: 484 entries, two valid ant prototypes. Selected native
  **Giant Ant**, PID 16777598, FID 16777313, rotation 0; source tile -1 denotes
  registry metadata, not a copied map actor. Default team 1, AI packet 7.
- Added ant cell 16685; 6 HP and 5 AP. Fresh script metadata passed checks and
  was removed before running; final `sid=-1`, `scriptIndex=-1`, no queued events,
  map-local count 5 → 5, and no derived prototype-stat change in the tested run.
- Five trees and three loot positions are unchanged from the alcove proof.
  Native paths to every remaining local cell and all three initially reachable
  cave exits pass with the ant present.
- Encounter diagnostic SHA-256:
  `9205896eceed6b6762450465a3ecea097e47da9461aabc9e884faa8438cae069`.
  Sibling alcove SHA-256 remains
  `813886cd817ec2bc39159735c2cb83f9f4bdae34a0cd59a834f6321ae4f4edfe`.

The user reported success on the first interactive test. Its log confirms one
enemy placement, native Giant Ant AI activity (packet 7 is named Rat), one complete
native save/reload cycle with handlers 0..26, a cave load and normal exit. Preserved
that session and isolated slot hashes under `evidence-combat/`. The user's response
is aggregate confirmation of the requested combat/defeat and post-combat reload;
no screenshot or detailed combat description was captured.

That first log contained only one reload and no cave-to-temple return, so the same
isolated runtime was reopened for a fresh new game and the two missing checks.
The user confirmed the follow-up was done: a reload with the ant alive and cave
travel/return with the same single ant and five trees. The follow-up log confirms
one new placement event, ARTEMPLE → ARCAVES → ARTEMPLE loads, another full 27-handler
save/load cycle, `ARTEMPLE.SAV` reload and normal exit. Both processes exited zero.
All three new-game diagnostics match both native probes exactly. Preserved the
follow-up log/diagnostics, final summary and 31 isolated slot-file hashes in
`runtime/proof-encounter-play/evidence-followup/`; the combat session has its own
earlier log and slot hash manifest. User responses are aggregate confirmations,
without a screenshot or detailed description of the ant's turns/counts.

All 87 original files and pinned active/reference map hashes remain unchanged
after both gameplay sessions. The feature-OFF binary contains no `wasteland::`
symbols. **Result: the bounded seed-42/Narg encounter is validated for native ant
combat, live/dead persistence without duplicate spawns, loot access and cave
round trip with the pinned Steam assets.** This does not establish other seeds,
presets, full campaign compatibility or autonomous out-of-combat hostility.
The Director remains unchanged.

## Single-opening grove alcove — 2026-10-18

Implemented independent `grove-alcove.v1` in `src/wasteland/alcove.*`, a closed
`schemas/alcove.v1.schema.json`, offline validator and five synthetic seed fixtures.
New flags: `--wi-inspect-alcove=<seed>`, `--wi-alcove-seed=<seed>` and
`--wi-probe-alcove=<seed>`. The existing location and partition generators and
fixtures remain unchanged. Both layout schemas now anchor their canonical seed
regex for standard JSON Schema validators, matching the existing local validator's
full-match behavior. The Director remains unchanged.

The module requires five blockers around all but one neighbor of its interior
cell, a reachable opening, connected remaining local terrain, protected entry and
native cave routes, one interior loot object and two distinct exterior loot
objects. The opening receives no loot. Only this mode samples to radius six,
bounded to 127 cells; previous modes retain radius four. It shares native source
art/prototype checks, ordinary object placement, rollback and native path postchecks.
It retains inherited floor/roof art and writes only its independent module
diagnostic `wasteland-alcove.json` plus the native exit audit. These are not native
save/checkpoint data; no MAP writer or persistent entity/Director binding was added.

Feature-ON build and 2/2 CTest suites PASS, including 19 Python methods. Coverage
includes pinned replay, caller order independence, exact enclosure/one-opening
semantics, protected references, disconnected remaining terrain, exterior loot
capacity, overlapping/unknown loot, closed fields/versions/types, seed bounds and
runtime guards. All five original location and five partition fixtures still pass.
Synthetic seed-42 alcove SHA-256:
`3df95b1a73bb8392d706ee1080ebd9bd5932387905e4b3026dfd7c048e8cdf01`.
Feature-OFF engine also builds successfully.

Fresh copied runtimes `runtime/proof-alcove-probe` and `runtime/proof-alcove-play`
both completed `--wi-probe-alcove=42` with exit zero, no native save slot and
byte-identical diagnostics. Preserved each probe's log, plan, audit and provenance
under its ignored `evidence-probe/` folder. Native scope is Narg and the pinned
English Steam 303136 assets:

- 35 sampled local cells; entry 17488. Protected route cells: 16687, 16888,
  17088, 17288 and 17488.
- The same verified **Tree** source as the partition: PID 33555379, FID 33555390,
  source cell 17676, rotation 0.
- Interior 16886, opening 16887. Five added tree cells: 16686, 16885, 17085,
  17086 and 17087. Every remaining local cell and all three previously reachable
  cave exit paths pass native checks.
- Loot: stimpak inside at 16886, stimpak outside at 18089, money at 18292.
- Native alcove plan SHA-256:
  `813886cd817ec2bc39159735c2cb83f9f4bdae34a0cd59a834f6321ae4f4edfe`.

The interactive test is launched separately with `--wi-alcove-seed=42` after
preserving probe evidence; its actual new-game diagnostic matches both probes.
All 87 original installation files still match their recorded hashes, including
original saves/configuration. Runtime archive and raw active/reference map hashes
match the pinned recipe. The feature-OFF binary has zero `wasteland::` symbols;
final CTest and `git diff --check` pass.

The user responded **"it worked"** to the requested five-tree/opening inspection,
walking through the opening, interior stimpak and exterior loot pickup, cave round
trip, save/reload with five trees/no duplicate loot, and normal quit. This is the
user's aggregate confirmation; no screenshot or separate detailed visual
description was captured. Logs independently corroborate one alcove placement,
successful ARTEMPLE → ARCAVES → ARTEMPLE loads, all native save/load handlers 0
through 26, an `ARTEMPLE.SAV` reload and normal `Game Exit`. The process exited
zero. Preserved plan/audit/provenance/logs and all 31 isolated slot-file hashes in
ignored `runtime/proof-alcove-play/evidence/`. Original file hashes and pinned
active/reference map bytes still match after gameplay.

**Result: the bounded five-tree, single-opening alcove is validated for seed 42,
Narg and the pinned Steam assets, with byte-identical fresh native replay.** This
adds a small layout module, not new floor/roof art, a generated MAP writer or
whole-campaign validation. The Director remains unchanged. The previous two-tree
partition and copied-template gameplay proofs below retain their recorded scope.

## Two-tree partition proof — 2026-10-18

Added an independent bounded `entry-partition.v1` planner, `schemas/layout.v1.schema.json`,
offline diagnostic validation and five new synthetic seed fixtures in
`tests/fixtures/layout-seeds.v1.json`. The original location schema, generator
algorithm and five location fixtures remain unchanged. New opt-in modes are
`--wi-inspect-layout=<seed>` (asset-free), `--wi-layout-seed=<seed>` (interactive
copied runtime), and `--wi-probe-layout=<seed>` (bounded owned-asset native probe).
No Director changes or new native MAP serialization were added.

The layout planner selects two adjacent unoccupied cells, protects entry/current
native exit routes, preserves connectivity of all remaining local cells and
leaves at least three unreserved loot cells. Runtime selects only an existing
nearby generic scenery/wall source whose object and loaded prototype are
scriptless, blocking and single-hex, with no inventory, no no-save/no-remove flags,
and matching loadable art at most 256x256 with at least 32 nontransparent pixels.
Automap marker prototypes are explicitly excluded. It verifies every remaining
local cell and initially reachable native exit after creating props. Any failure
rolls back props/loot, removes diagnostics and returns interactive layout startup
to the menu with an explicit failure notice. The source PID is native audited
metadata; no external PID enters engine creation.

Feature-ON build and 2/2 CTest suites PASS; Python coverage is now 15 methods.
Added replay/canonical ordering, protection/connectivity/capacity, bad references,
unknown/version/type rejection, invisible/oversized-art regression and
opt-in/isolated-runtime tests. Feature-OFF build PASS. All original location
seed fixtures still pass.
Synthetic seed-42 partition fixture SHA-256:
`b5f172d00058c3904bdf1ea30d160c75693fe44be036a759b3ea007f05aab5ee`
(blockers 20700 and 20701; synthetic cells, not an ARTEMPLE claim).

Initial trials exposed invisible collision art: the first filter selected
`SceneryProtoTypeId::BlockingHexAutomap` (PID 33554776), and the wall-only trial
had no visible eligible source, safely canceling placement. Neither trial is
counted as a successful proof. Native probe diagnostics established the corrected
visible generic-scenery source and added the pixel/marker exclusions above.

The native probe initializes Narg via the existing `premade\\combat.gcd` path,
uses the normal native new-game load/hooks, checks placement and then cleans up
without running a gameplay input loop or creating a save slot. It still requires
SDL and owned assets; it is not a visual/gameplay test. Failed placement returns
a nonzero exit code, rather than opening an unchanged map as a completed proof.

Two fresh successful probes (`runtime/proof-layout-probe-visible` and
`runtime/proof-layout-replay`) produced identical layout, loot and native source
metadata. The interactive `runtime/proof-layout-visible` output matches both:

- Source: native **Tree**, PID 33555379, FID 33555390, source hex 17676, rotation 0.
  Native art is 62x141 with 2108 nontransparent pixels; source/prototype are
  scriptless, single-hex blockers with native save/removal capability.
- Added tree cells: 17287 and 17487, beside entry 17488. Protected route cells:
  16888, 17088, 17288 and 17488. All three initially reachable cave exit paths
  and all remaining local cells pass native path postchecks.
- Layout SHA-256:
  `25a50cb784e901466060ca00c1cca229e50f78daaf0cf3d3d7f6e76e9671bcef`.
- Partition location SHA-256:
  `f9803e10cbb68a55e78474623f1622dc944dc228d8fc185f2f6a08d9510be269`.
  Its separate template ID is `engine.artemple.partition.v1`; the original
  `engine.artemple.v1` and synthetic location fixtures are retained.

The user described **two new trees** and confirmed the requested movement,
three pickable loot objects, cave round trip and save/reload checks worked.
Logs corroborate one layout event, all 27 native save/load handlers and normal
exit. Active/reference map bytes and all 87 original installation files remain
unchanged. Preserved plans, audit, source log and isolated slot hashes in ignored
`runtime/proof-layout-visible/evidence/`.
Final post-hardening native probe exited zero and regenerated diagnostics matching
the gameplay run. Feature-OFF build has zero `wasteland::` symbols; final CTest
run passed 2/2 suites with 15 Python methods, and `git diff --check` passed.

**Result: the bounded two-tree obstacle cluster is validated for seed 42, Narg
and the pinned Steam assets, with matching fresh native probe replay.** This is
not a generated room, full dungeon or campaign compatibility claim. The Director
remains unchanged; the previous copied-template proof below remains validated.

## Copied-template proof — 2026-10-18

Added `tools/wasteland/stage_template.py`, pinned recipe
`templates/artemple-copy.v1.json`, opt-in `--wi-template-seed=42`, and a separate
closed `schemas/template-audit.v1.schema.json` diagnostic contract. No location.v1
or generator algorithm/version changes were made; all five pinned fixtures still
pass. Native map names, registry indices, scripts, tiles and save handlers remain
unchanged. No Director functionality was added.

Staged the owned English Steam 303136 archives into a new ignored
`runtime/proof-template`, extracting unchanged maps with the native DAT tool.
The recipe pins all three archive hashes plus these map hashes:

| Copied map | Bytes | SHA-256 |
| --- | ---: | --- |
| ARTEMPLE.MAP | 92780 | `11fc44f3fad558f3cb9e12bf41018a023ef7de8def365d516a44336157fec050` |
| ARCAVES.MAP | 526972 | `ba35474b8ea67aec50b6a962a1bc767a87366449d4b631fd1b4ce241847b5bfe` |

Unknown archive versions, extra patches and loose overrides of those maps are
refused. Reference copies and provenance are stored beside the runtime; startup
requires the copied-template marker and byte-identical active/reference maps.
These are isolation/integrity checks, not an authenticity boundary against
someone modifying both copies. Native saved maps remain upstream-owned.

The engine audits real exit-grid objects before loot placement, without moving
the player or changing transitions. The report validates against the new schema:

- Entry: hex 17488, elevation 0, rotation 5.
- Eighteen exit grids have resolvable native map/file references and bounded
  destination fields. Three reach `arcaves.MAP` (registry index 3) at hex 28707,
  elevation 0, rotation 0: source hexes 15686, 15885 and 15887, path lengths 10,
  10 and 9 respectively.
- Fifteen other grids target `arvillag.MAP` (index 4), destination hex 11683.
  Native pathfinding returns zero for them at initial entry; they are not
  reachable/validated routes in this proof. Resolving a destination reference
  alone does not prove traversal or campaign progression.

The user confirmed all three seeded loot objects, temple/cave travel and return,
music and native save/reload without duplicate loot, then quit normally. Logs
corroborate one WI placement event and all 27 native save and load handlers,
including successful `ARTEMPLE.SAV` restoration. The copied-map location JSON is
byte-identical to the original seed-42 diagnostic (SHA-256
`20c57df442a3eed7a9bb58f9495193a9c4196bf539fa3b825320ddb23c25be40`).
Both active map copies and references still match pinned map hashes after play.
All 87 original installation file hashes remain unchanged, including original
saves/configuration. Preserved audit, location, provenance and isolated slot hash
evidence under ignored `runtime/proof-template/evidence/`.

Automated checks: feature-ON build and 2/2 CTest suites PASS; explicit Python
discovery with `WI_ENGINE` and `WI_DAT_TOOL` runs 13/13 methods with no skips.
New coverage includes synthetic native DAT extraction, source/save preservation,
wrong asset/map hashes, extra patches, loose overrides, copied-map mutation/link
rejection and negative audit-schema cases. Feature-OFF build PASS with zero
`wasteland::` symbols. No game data is tracked in Git.

**Result: the unchanged copied temple/cave proof is validated for seed 42, Narg
and the pinned assets.** This does not validate the initially unreachable village
exits, new layouts, generated MAP serialization, other seeds/presets or an expanded
Director. The earlier completed loot-overlay validation remains intact below.

## Runtime validation continuation — 2026-10-18

Resumed the existing checkout with the six previously modified files preserved.
The first proof session had exited normally (`Game Exit` in its log); no engine
process was running. The first run's user-confirmed placement, movement, combat,
inventory, exits and save/reload results below remain the evidence for that run.

Verified the fresh `runtime/proof-replay` archives against the original staging
hashes. No previous saves, generated map caches, symlinks or placement diagnostic
were present, and all 21 music files were staged. Launched the existing feature-ON
engine there with `--wi-dev-seed=42`. Startup reached game initialization with an
empty `launch-desktop.log`. After NEW GAME with the same Narg preset, the fresh
`ARTEMPLE.MAP` load succeeded and the generated location passed the Python contract.
Its complete JSON is byte-identical to the preserved first runtime diagnostic:
SHA-256 `20c57df442a3eed7a9bb58f9495193a9c4196bf539fa3b825320ddb23c25be40`.
The replay therefore reproduces both stimpaks and the money placement at the
same hexes. Preserved replay output and comparison evidence in ignored
`runtime/proof-replay/evidence/`. No missing-map-music error appears in this run's
log. Replay is verified through the diagnostic and engine log; there is no separate
user confirmation of replay visuals. Audible music is confirmed in the disabled
comparison below. The Director remains unchanged.

The replay subsequently exited normally. Its log also records successful temple
and cave transitions with only the original WI placement event. Launched the
feature-OFF engine from the separate `runtime/proof-legacy` copy, without WI
arguments. It initialized successfully with an empty launch log. Rechecked that
the binary has zero `wasteland::` symbols and the disabled runtime has no
`wasteland-location.json`. Requested user confirmation that the WI loot is absent,
music is audible, and movement, inventory, combat, exits and save/reload work.
The user confirmed all those checks passed and the game was closed. The log
corroborates successful temple/cave loads, combat execution, save/load handlers
0 through 26, an `ARCAVES.SAV` reload and normal `Game Exit`. The disabled session
has zero WI placement events and no placement diagnostic. Its isolated slot is
`runtime/proof-legacy/data/SAVEGAME/SLOT01`. All 87 original installation files
still match their pre-staging hashes after this final comparison.
Preserved all 31 disabled slot file hashes and the final evidence summary in
ignored `runtime/proof-legacy/evidence/`. Confirmed both native save and load
handler sets are exactly 0 through 26. Final `./tools/wasteland/test.sh`: PASS,
2/2 CTest suites, including all 9 Python methods. `git diff --check`: PASS.

**Result: the bounded seed-42 ARTEMPLE loot-overlay runtime validation is complete.**
This covers the user-confirmed enabled gameplay/save checks, byte-identical fresh
runtime replay, and user-confirmed disabled gameplay/music/save checks with this
Steam asset version and Narg preset. It does not establish whole-campaign or
other-template/seed compatibility, and adds no Director checkpoint binding.
The Director remains unchanged. Historical pending statements in the dated
sections below describe earlier stages, not the current result.

## Transferred checkout verification on Josh — 2026-10-16

Verified checkout: `/home/josh/Projects/WastelandInfinite/engine`. The parent
workspace is not the engine repository. The working tree was clean on entry;
branch `wasteland-infinite` is at `a77cb4f27b16895d5ce1e63193e5822a074d1811`,
with `463fb42d` and `823df4f6` present and `wi-upstream-baseline` pointing to the
pinned upstream SHA below. `AGENTS.md` is tracked. `git fsck --full` passed.

`git bundle verify /home/josh/WastelandInfinite.bundle` passed and reported
complete history with the expected branch and baseline tag. The local `origin`
points to that bundle, not the GitHub upstream; no remote was changed or pushed.

Local environment: Linux Mint 22.3 x86-64, GCC 13.3.0, CMake 3.28.3, Ninja 1.11.1,
Python 3.12.3, SDL2 2.30.0 and zlib 1.3.

- `./tools/wasteland/test.sh`: PASS, 2/2 CTest suites. CMake regenerated the local
  feature-ON Release build; Ninja reported existing targets current.
- Explicit Python unittest discovery with `WI_ENGINE` set: PASS, 9/9 methods,
  including all five pinned seed fixtures.
- Actual engine `--wi-inspect=42`: PASS from an empty temporary directory.
  Repeated output was byte-identical, matched the seed-42 SHA-256 below, and
  created no files in that directory.
- Engine, core test executable and ce-dat-tool are native Linux x86-64 binaries.
- Fresh `WASTELAND_INFINITE=OFF` Release configure/build in `out/build/wi-legacy`:
  PASS. The resulting native Linux x86-64 engine has zero `wasteland::` symbols
  under `nm -C`. Existing upstream warnings remain; build log:
  `/tmp/wi-josh-legacy-build.log`.
- `git diff --check`: PASS after recording these results. Only this validation
  document changed; no engine, Director, schema or fixture changes were needed.

Runtime gameplay validation remains pending. No owned assets were staged or
graphical gameplay launched in this verification. Loot visibility/pickup, movement,
actual exits, combat, inventory, native save/reload, runtime replay and disabled-mode
campaign compatibility remain unverified. The Director remains unchanged and its
deferred milestones remain gated on runtime placement validation. The historical
asset search described below was on Hermes; no new asset search was performed here.

## Owned-asset staging and desktop launch on Josh — 2026-10-16

After checkout verification, found the local Steam Fallout 2 installation at
`/home/josh/.local/share/Steam/steamapps/common/Fallout 2` (Steam build ID `303136`).
Used `tools/wasteland/stage_runtime.py` to create the new ignored `runtime/proof`.
All 87 source files had identical SHA-256 hashes before and after staging,
including original configuration and saves. No source saves, `.sav`/`.bak` files
or symlinks were staged. The three copied archives matched their source hashes:

| Archive | SHA-256 |
| --- | --- |
| master.dat | `9b096d3035edafd4077deeb8ee7877a803b9db98497aa4596624b5c058a84711` |
| critter.dat | `da83d615967c0bac41fbc50e6b86b3088401ff69ed9d8b2e5be1d6ac0c125bcd` |
| patch000.dat | `16122a47d9fa1fb2f490d1fb38bb1b7eb8ca8edcf2c06a2c9d78beed812ff564` |

The actual source contained generated `data/worldmap.dat` and
`data/maps/AUTOMAP.DB` caches. Added exclusions for these paths and extended the
existing staging regression test to verify exclusion and source preservation.
Removed those two copied caches from the staged runtime before launch.
`./tools/wasteland/test.sh` still passes 2/2 suites, including all 9 Python methods.

Built `ce.dat` from the tracked CE UI files and enabled `[debug] mode=log` only in
the isolated runtime config. Local staging evidence is in ignored
`runtime/proof/staging-evidence.json`.

The initial sandboxed launch logged an audio permission error and was interrupted.
Relaunched the actual engine with desktop access from `runtime/proof` using
`--wi-dev-seed=42`. `debug.log` confirms game data and CE UI loading and
initialization through combat, automap and messages; `launch-desktop.log` is empty.
This establishes startup only. Native UI viewing/control is unavailable in this
chat; requested the user's direct visual/pickup observations after NEW GAME.
The user selected NEW GAME and confirmed all three generated objects are visible
and pickable. The log records `MAP LOAD: ARTEMPLE.MAP rc=0` and successful placement
of `loc-000000000000002a-engine.artemple.v1`. Its diagnostic passes the existing
Python location contract. It contains two stimpaks (hexes 16888 and 17487) and
money (hex 17085), with entrance/return anchor 17488 and 24 reachable cells.

Runtime seed-42 diagnostic SHA-256:
`20c57df442a3eed7a9bb58f9495193a9c4196bf539fa3b825320ddb23c25be40`.
Geometry SHA-256 (sorted compact JSON of template, elevation, walkable, obstacles,
reserved, entrances and exits):
`bc7671f6682a5b48711cbbc2153e99efc927b8342a5cc2a2f78b0d991166db20`.
Preserved the first diagnostic at ignored
`runtime/proof/evidence/seed42-first-location.json`; this is distinct from the
synthetic fixture and is not a native save/checkpoint.

The first launch reported missing map music. Steam stores its music outside
`data/`, so staging now also copies optional `sound/` and sets relative music
paths. Both loose trees reject symlinks before staging starts. Extended the
staging test to cover music copying and sound symlink rejection; all tests still
pass. Copied and hash-verified all 21 music files into the active isolated runtime;
audible playback still needs confirmation on a subsequent music load. All 87
original source file hashes were also rechecked unchanged after game startup.

The user subsequently confirmed movement, inventory, combat, temple exits and
native save/reload all passed, with uncollected loot retained and no duplicate
loot on load. These are direct user observations; native UI viewing/control
remains unavailable to the agent. The isolated slot is
`runtime/proof/data/SAVEGAME/SLOT01`. Logs confirm a successful saved
`ARCAVES.SAV` load and completion of all 27 native load handlers, with only one
successful WI placement log entry across the session. Preserved a SHA-256 manifest
of all 31 isolated slot files in `runtime/proof/evidence/native-slot01-sha256.json`;
this evidence manifest adds no Director checkpoint or save format.
Rechecked all 87 original installation file hashes after gameplay/save/reload:
unchanged, including original saves and configuration.

Repeated fresh-run determinism, the disabled comparison and audible music after
the staging repair remain pending. No Director changes were made.

Prepared two additional isolated copies with the corrected staging tool:
`runtime/proof-replay` and `runtime/proof-legacy`. Both have 21 music files, fresh
relative configuration and logging, no saves/caches/symlinks, and have not yet
been launched. After finishing and quitting the current session, the remaining
comparison launches are:

```sh
# Fresh seed replay: select NEW GAME and the same Narg preset.
(cd runtime/proof-replay && ../../out/build/wi-dev/fallout2-ce --wi-dev-seed=42)
cmp runtime/proof/evidence/seed42-first-location.json runtime/proof-replay/wasteland-location.json

# After quitting replay, repeat the gameplay checks with WI compiled out.
(cd runtime/proof-legacy && ../../out/build/wi-legacy/fallout2-ce)
```

These are pending steps, not successful validation results.

The following sections preserve the original Hermes validation record.

## Source and environment

Origin: https://github.com/fallout2-ce/fallout2-ce.git
Upstream: `6de8078f02884db8d006bfff490ba05f3af5aabc`, tag `wi-upstream-baseline`.
Branch: `wasteland-infinite`; baseline-fix commit: `823df4f6`.
Mint 22.3 / x86-64, GCC 13.3, CMake 3.28.3, Python 3.12.3, system
SDL2 2.30.0 / zlib 1.3. No extra Python packages, API keys, downloads or game
assets are required by the automated tests. Existing upstream warnings remain.

## Results

- Unmodified configure: PASS. Unmodified compile: FAIL at the upstream enum
  regression, recorded in BASELINE.md. Baseline with the narrow type fix: PASS.
- WASTELAND_INFINITE=ON Release engine, core test executable and ce-dat-tool:
  PASS. All produced executables are native Linux x86-64.
- WASTELAND_INFINITE=OFF Release engine: PASS; `nm -C` finds no `wasteland::`
  symbols. This verifies compilation exclusion, not original campaign gameplay.
- `./tools/wasteland/test.sh`: PASS, 2/2 CTest suites.
- C++ suite: uint64 boundaries; RNG isolation/bounds; neighbor reciprocity for
  every grid cell/direction; deterministic replay; canonical input ordering;
  invalid hex/elevation/capacity; disconnected terrain; overlaps; prototype and
  placement identity; disabled/inspect/invalid options; runtime config/link guard.
- Python suite: 9/9 unittest methods, covering C++ JSON/schema and reserialization,
  five golden seed fixtures, version/type/unknown-field/reference/connectivity/
  overlap rejection, strict bounded JSON, malformed options, isolated runtime
  requirements, and synthetic asset staging without source/save modifications.
- Headless engine inspector executed repeatedly from empty temporary directories;
  no files are created there. Final script run completed in under a second with
  compiled targets already current. Full engine builds take longer.
- `git diff --check`: PASS. Tests created no real runtime or proprietary assets.

Five seeds: 0, 1, 42, 9007199254740993 and 18446744073709551615.
Seed 42 serialized fixture SHA-256:
`07eef3fe9f75e98bf844f3eba0851842c798a4cfb012b50744254b1a39980e5c`.
The fixture JSON contains expected spawn lists and output digests; do not
regenerate those silently when behavior changes.

## Reproduce

From the existing engine/ checkout:

```sh
./tools/wasteland/test.sh
./out/build/wi-dev/fallout2-ce --wi-inspect=42
ctest --test-dir out/build/wi-dev --output-on-failure

cmake -S . -B out/build/wi-legacy -G Ninja -DCMAKE_BUILD_TYPE=Release -DFALLOUT_VENDORED=OFF -DWASTELAND_INFINITE=OFF
cmake --build out/build/wi-legacy --target fallout2-ce -j 4
```

For complete Python test output (also present in CTest's LastTest.log):

```sh
WI_ENGINE="$PWD/out/build/wi-dev/fallout2-ce" python3 -m unittest discover -s tests -p 'test_*.py' -v
```

Owned-asset staging / runtime commands are in README.md. CMake may report Git
metadata differently after local commits; this does not affect seed fixtures.
Ignored local build logs: out/build/wi-baseline/{build,fixed-build}.log;
session build/test logs: /tmp/wi-{build,incremental,legacy-build,final-test}.log.

## Files created/changed

Modified upstream files: CMakeLists.txt, src/main.cc, src/win32.cc,
src/party_member.cc, README.md, .gitignore. No legacy registry/save format edits.

Added: src/wasteland/{CMakeLists.txt,location.h,location.cc,options.h,options.cc,
runtime.h,runtime.cc}; director/__init__.py and module package markers;
director/world/contracts.py; schemas/location.v1.schema.json;
templates/catalog.v1.json; tests/{location_test.cc,test_contract.py,test_staging.py,
fixtures/seeds.v1.json}; tools/wasteland/{test.sh,stage_runtime.py}; assets/README.md;
upstream.lock.json; .env.example; AGENTS.md; ARCHITECTURE.md; ROADMAP.md;
ENGINE_INTEGRATION.md; docs/wasteland/{BASELINE.md,VALIDATION.md}.

## Blocking issues and unverified work

Owned game archives were not found in the scanned project/home/standard game
locations. No rendered engine session, actual loot visibility/pickup, template
safety, native save/load or original campaign compatibility has been validated.
The runtime adapter is compiled, not proven in gameplay. Do not call this a
successful playable prototype.

Director HTTP/providers/SQLite journal, NPC memory/factions, quest execution and
save/checkpoint binding are deferred. Consequently duplicate-event, quest-graph,
checkpoint-recovery and provider-outage tests do not exist yet. See ROADMAP.md
for the explicit test matrix; 2/2 does not mean all future requirements passed.
