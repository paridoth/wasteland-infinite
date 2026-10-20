# Wasteland Infinite engine integration audit

Modified derivative of Fallout 2 Community Engine; upstream and license are in
`upstream.lock.json` and `LICENSE.md`. Audit is against upstream revision
`6de8078f02884db8d006bfff490ba05f3af5aabc`, not inferred from historical interfaces.

## Implemented hooks

| File / function | Verified behavior and use |
| --- | --- |
| `src/win32.cc`, `main` | Platform entry on Linux too. The opt-in preflight runs before SDL_ShowCursor, autorun locking, video/audio initialization and game configuration. Inspector exits here; no assets needed. |
| `src/main.cc`, `_main_load_new` | Upstream calls mapInit, then mapLoadByName before world-map positioning/music. Enabled builds call populateNewGame only after mapLoadByName returns 0. Disabled builds retain the original call. No hook on native loads or transitions. |
| `src/wasteland/runtime.cc`, `populateNewGame` | Requires fresh ARTEMPLE.MAP, rejects MAP_HEADER_SAVED. Bounded local BFS collects reachable, empty cells near the real gDude spawn. Generates three loot placements, resolves approved prototype enums, checks scriptless prototypes and loadable art, creates objects, and rolls back created objects on failure. |
| `src/party_member.cc`, `partyMemberPlaceMultihex` | Separate baseline build repair: local elevation remains MapElevation rather than converting to int. No new gameplay logic here. |

The build flag `WASTELAND_INFINITE` defaults OFF. In an enabled build,
`--wi-inspect=42` prints a synthetic location contract; `--wi-dev-seed=42` enables
the provisional new-game hook after checking an isolated runtime directory.
Without a WI argument the enabled build follows ordinary startup and the hook
returns immediately. A map load failure keeps the upstream error handling.

`--wi-template-seed=42` uses the same generator/new-game hook on pinned, unchanged
owned map copies staged by `tools/wasteland/stage_template.py`. Preflight requires
both maps to match staged references. `auditCopiedTemplate` snapshots exit-grid
objects using `isExitGridProtoId`, then resolves `data.misc` destinations through
`wmMapIdxToName`, checks VFS availability and scalar bounds, and calls `_make_path`
for active exits on the player's elevation. It writes a separate versioned audit
before creating loot and refuses placement if no valid reachable exit exists.
It caps the audit at 128 grids, never moves the player to test an exit, and leaves
all native registry entries, scripts and transition code intact. Destination
reference validity is not proof that the destination tile is traversable; the
tested temple/cave round trip also has direct user confirmation.

`--wi-layout-seed` requires the same copied-template guard and adds two adjacent
static blockers near entry. `_make_path` rotation buffers (bounded to the native
800-step capacity) reserve all initially reachable exit routes. The asset-free
planner preserves local connectivity and free loot capacity. Runtime verifies
source object/prototype scriptlessness, single-hex blocking flags and loadable
visible art before creating any props. It accepts only generic scenery/walls
already present within 12 hexes of entry, explicitly excludes automap markers,
and rejects no-save/no-remove flags. Candidate ordering is deterministic. After
placement it rechecks paths to all remaining local cells and all baseline exits.
Failure rolls back all created props/loot and removes this attempt's layout and
location diagnostics. Native script dispatch, registries and save handlers are
unchanged; layout plans never supply arbitrary prototype IDs to creation.

The compile-guarded `--wi-probe-layout` branch in `falloutMain` initializes the
existing `premade\\combat.gcd` Narg preset and native new-game hooks, reads the
placement status, cleans up and exits without a game loop. It requires copied
runtime preflight and does not load or create a save slot. The interactive layout
branch also checks placement status and returns to the menu on failure, rather
than treating a canceled overlay as a successful gameplay proof. WI-disabled
builds exclude these branches.

`--wi-alcove-seed` adds an independent five-prop, single-opening grove module.
Alcove-based modes sample a radius-six occupancy graph (maximum 127 cells); the
older modes retain their radius-four sampling. `grove-alcove.v1` reserves native
routes, leaves one neighbor of the interior open, puts exactly one allowlisted
loot object inside and two outside, and leaves the opening empty. It shares the
audited prop source and native postchecks above. `wasteland-alcove.json` records
the module and its loot; it does not alter location.v1 or act as a checkpoint.
`--wi-probe-alcove` uses the same Narg initialization/cleanup path. Both probes
report failure through their exit status; both interactive layout modes return
to the menu on failure. Source selection is native metadata, never JSON PID input.

`--wi-encounter-seed` composes the same alcove with one seeded, blocking ant.
`--wi-inspect-encounter` is asset-free; `--wi-probe-encounter` uses the normal
bounded Narg native probe. The new planner reserves the opening, all loot and
native routes, excludes physical hex radius three around entry, and preserves
connectivity with the enemy cell removed. Native postchecks repeat after creating
the ant. The versioned `wasteland-encounter.json` is independent of the alcove and
location contracts; its offline validator can check exact paired geometry.

The ant comes from a bounded scan of the native critter prototype registry, not
a generated PID. Source checks require giant-ant kill type, single-hex/saveable
flags, non-party identity, visible idle art, hostile native team and an exact AI
packet match. `objectCreateWithProtoId` initializes HP/AP, and temporarily
recomputes shared derived stats; the adapter immediately restores the original
`CritterProtoData`. Creation also allocates prototype script metadata without
executing its program. The adapter requires a fresh null program, default flags,
matching owner/index and no allocated map locals or queued events, then removes
the script via `scriptRemove` and clears `sid`/`scriptIndex` before placement or
rollback. Native map-local count must remain unchanged. It does not strip scripts
from existing actors or modify shared prototype script/team/AI values.

Only the generated ant is scriptless; normal engine AI handles its turns once
the player initiates combat. Combat behavior and live/dead persistence require
the separate rendered gameplay test. No Director hooks, native save-handler
changes, registry reordering or raw MAP serialization are introduced.

## Map loading, tiles and native serialization

- `src/map.cc`: `mapLoadByName(char*)` MODIFIES its input (uppercases it and
  temporarily changes .MAP to .SAV). It prefers existing saved map state, then
  calls static `mapLoad(File*)`; do not pass string literals or immutable storage.
- `mapLoad` calls `mapHeaderRead`, local/global variable loaders, `_square_load`,
  `scriptLoadAll`, then `objectLoadAll`. It sets elevation, player tile/rotation,
  map script, executes start/map-enter/map-update scripts, and sets header
  version 20. Inserting placements before those scripts could invalidate them.
- `mapHeaderRead` / `mapHeaderWrite` explicitly serialize fields. `MapHeader`
  includes 44 reserved integers in `src/map.h`. Struct packing is NOT the on-disk
  contract. No new binary writer is implemented and no reserved fields are guessed.
- `src/map_defs.h`: 200 x 200 hex grid, 100 x 100 square grid, three elevations.
  Hex object positions and square art positions are different coordinates.
- `src/map.h`, `TileData`: each square's `tileFid` contains floor and roof short
  identifiers. `src/mapper/map_func.cc`, `placeTile` preserves their flags using
  `FloorTileFrmId`, `RoofTileFrmId`, `TileFrmId`; this is the supported pattern for
  later tile edits. Current proof INHERITS both selections from the template.
  Art-list tile indices must be verified from owned data before generated tiles.
- `src/tile.cc`, `tileInit` initializes `_dir_tile` according to column parity;
  `tileGetTileInDirection` clamps at `tileIsEdge`. Our offline neighbor math is
  based on those actual offsets, rejects row wrapping, and is compared to the
  engine neighbor at runtime. Tests check reciprocity for all 40,000 hexes.
- `src/map.cc`, `_map_save_in_game`, `_map_save`, and map save serialization remain upstream-owned.
  Generated objects use ordinary object serialization; no .MAP/.SAV files are
  written directly by the generator.

## Object and encounter placement

- `src/object.cc`, `objectCreateWithProtoId(Object**, const ProtoId&)` resolves
  `protoGetProto` and delegates to `objectCreateWithFrmIdProtoId`. Creation can
  attach a prototype script. The adapter rejects scripted prototypes first.
- `objectSetLocation(Object*, int, MapElevation, Rect*)` validates grid/elevation
  and manages spatial lists/light. Never mutate an object's tile directly.
- `objectDestroy` is used for rollback. Prototype flags, rotation, scripts and
  inventory require engine initialization; raw memory copying is not a spawn API.
- `src/proto_types.h`: verified enums `ItemProtoTypeId::Stimpak` and `Money` are
  the only current mappings. No arbitrary PID from JSON or LLMs reaches creation.
  `FrmImage::lock(FrmId(proto))` verifies the referenced art can actually load.
- `objectFindFirstAtLocation`, `_obj_blocking_at` reject occupied cells before
  generation; `_make_path` in `src/animation.cc` returns path length or ZERO for
  failure. We require a positive path, not a guessed negative-error convention.
  Generated loot is nonblocking; entrances/return anchors cannot be occupied.
- `src/worldmap.cc`, `wmSetupRandomEncounter` is called by map loading and uses
  encounter tables; `wmEvalTileNumForPlacement` tests blockers/pathfinding.
  `wmParseMapsConfig` / `wmParseAreasConfig` build map/area registries.
  `wmMapIdxToName`, `wmMapMatchNameToIdx` and native saves depend on their indices.
  Do not reorder registries or replace global RNG calls. New world topology will
  need an independent stable-ID mapping into verified registry entries.

## NPC events, dialogue and lifecycle (audited, no hooks yet)

- `src/scripts.cc`, `scriptExecProc(int, ScriptProc)` is the central dispatch
  boundary. `scriptsRequestDialog` queues a target; `scriptsHandleRequests`
  later processes requests on the engine thread. Observe validated results at
  a defined safe boundary; do not execute arbitrary model-produced scripts.
- Script list positions and `Script::index` are compatibility-sensitive.
  `scriptsNewObjectId` creates native runtime IDs; they are not stable campaign
  identity. A future adapter needs explicit UUID/native-object bindings.
- `src/game_dialog.cc`, `gameDialogEnter`, `gameDialogSetTextReply`,
  `gameDialogAddTextOptionWithProc` / `gameDialogAddTextOption` are verified UI
  boundaries. They depend on active speaker, interpreter and modal UI state.
  Build a separate test conversation before routing original dialogue through
  them. No existing dialogue has been replaced.
- `src/game.cc`, `gameInitWithOptions`, `gameReset`, `gameExit` define lifecycle.
  `gameHandleKey` participates in normal/combat input; `src/main.cc`, `mainLoop`
  processes scripts, transitions and rendering. Future network workers must post
  bounded validated commands; the engine thread alone mutates objects. No
  synchronous Director work belongs in any of these loops.

## Save integration strategy (not wired yet)

`src/loadsave.cc` uses 27 ordered handlers in `_master_save_list` and
`_master_load_list`. `lsgPerformSaveGame` writes SAVE.DAT plus native map/prototype
files, `sfallgv.sav` and `ce.sav`; `lsgLoadGameInSlot` restores them. `src/ce_save_game.cc` / `src/ce_save_game.h`
implements CE's existing extension file. `_patches` determines the native save
root; staging sets both patch roots to the isolated runtime's real `data/`.
Do not append undocumented bytes or reorder handlers.

Planned WI binding: a separate versioned `wasteland.json` sidecar for each
compatible native slot, containing immutable random campaign UUID, generator and
schema versions, checkpoint UUID, journal head and a digest manifest of the
completed native files. No sidecar means a legacy save, not the newest WI state.
Seed is NOT campaign identity; two campaigns may share a seed.

Use an SQLite transaction to create an immutable checkpoint of the journal branch
and projections, then stage/fsync the sidecar and atomically rename it only after
all native save components succeed. On crash, missing/mismatched manifests must
fail closed or recover the previous completed checkpoint. Native multiple-file
writes and SQLite are not one atomic transaction: explicitly handle the gap.
Load validates the sidecar before restoring the exact journal head/projections.
Loading an old save creates/continues a branch; later immutable events must not
leak into that projection. Test with synthetic native files before adding hooks.

The current `wasteland-location.json` is a diagnostic only, not a checkpoint.
In the isolated seed-42 proof, the user confirmed native save/reload retained
uncollected placed loot without duplicates; logs confirm all 27 load handlers
completed. Fresh-run runtime diagnostics also matched byte-for-byte. This is
bounded smoke-test evidence, not full campaign compatibility. No Director state
exists which could safely be bound to native saves yet. See the
[validation record](docs/wasteland/VALIDATION.md) for the completed disabled-build
comparison, tested assets/preset, scope and distinction from future checkpoints.

## Reference trail

Primary authority for this audit is the pinned checked-out source. Additional
references inspected: [Fallout 2 CE](https://github.com/fallout2-ce/fallout2-ce),
[historical upstream](https://github.com/alexbatalov/fallout2-ce),
[rotators MAP documentation](https://github.com/rotators/fallout2-docs/blob/master/content/pages/map.md),
[savegame documentation](https://github.com/rotators/fallout2-docs/blob/master/content/pages/savegame.md),
[modding documentation](https://github.com/JanSimek/fallout2-modding),
[sfall](https://github.com/sfall-team/sfall).
No external binary-field descriptions or guessed IDs were used to write data.
