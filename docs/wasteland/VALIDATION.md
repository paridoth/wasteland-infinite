# Wasteland Infinite session validation — 2026-10-16

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
