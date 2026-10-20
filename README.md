# Wasteland Infinite — experimental Fallout 2 CE modification

Modified for Wasteland Infinite; upstream attribution and Sustainable Use License
are preserved below and in LICENSE.md. Personal/noncommercial development.

This isolated checkout contains a working build, an **asset-free procedural
proof**, and a **runtime-validated seed-42 loot overlay on ARTEMPLE.MAP** using
isolated copies of locally owned Fallout 2 data. The user confirmed visible/pickable
loot, movement, inventory, combat, exits and save/reload without duplicates. A fresh
run reproduced the runtime diagnostic byte-for-byte; the disabled-build comparison
also passed gameplay, music and save/reload checks. This validates the bounded
proof with the tested assets/preset, not whole-campaign compatibility.
The original campaign and its native systems remain the engine foundation.

```sh
# Mint 22.x: documented dependencies (already installed on the development host)
sudo apt install build-essential cmake ninja-build libsdl2-dev zlib1g-dev python3

# Build the opt-in engine and run all asset-free tests; no downloads/API key
./tools/wasteland/test.sh

# Actual engine executable, headless synthetic location inspection
./out/build/wi-dev/fallout2-ce --wi-inspect=42
```

The default build keeps the feature OFF:

```sh
cmake -S . -B out/build/wi-legacy -G Ninja -DCMAKE_BUILD_TYPE=Release -DFALLOUT_VENDORED=OFF -DWASTELAND_INFINITE=OFF
cmake --build out/build/wi-legacy --target fallout2-ce -j 4
```

To try placement **only in a separate copy** of owned vanilla data:

```sh
python3 tools/wasteland/stage_runtime.py /path/to/owned/Fallout2
(cd runtime/proof && ../../out/build/wi-dev/fallout2-ce --wi-dev-seed=42)
# Select NEW GAME. Inspect three loot objects near the starting entrance.
```

Staging refuses an existing destination, copies archives/loose data and sound, excludes
saves/caches, creates fresh relative config and builds optional CE UI data from
upstream's tracked files. Original data/configuration are only read. Development
mode requires that isolated runtime, rejects save/config command-line overrides,
and only populates a fresh ARTEMPLE.MAP. The synthetic inspector fixture is not
that map. `wasteland-location.json` is diagnostic output, not a campaign save.
WI test modes skip the Elder movie on NEW GAME. Fresh staged runtimes default
to running; Shift retains the native temporary walk behavior.
No HTTP service, persistent Director, NPC memory or generated quests is claimed yet.

The next proof uses unchanged copies of the pinned temple/cave maps, retaining
native map names, registry indices, scripts and floor/roof art:

```sh
python3 tools/wasteland/stage_template.py /path/to/owned/Fallout2
(cd runtime/proof-template && ../../out/build/wi-dev/fallout2-ce --wi-template-seed=42)
```

This accepts only the recorded English Steam 303136 archive hashes. It extracts
with the native DAT tool, records SHA-256 provenance, and keeps reference copies
under the ignored runtime. Startup rejects missing/changed map copies. Before
loot placement, `wasteland-template-audit.json` records actual native exit grids,
registry destinations and current pathfinding from entry. This diagnostic is
separate from the location contract's return-to-start anchor and is not a save
checkpoint. The copied temple/cave round trip, music and save/reload passed the
user's seed-42/Narg check; the village exits were not reachable in the initial
audit and have not been validated by this proof.

An experimental two-object partition uses the same pinned copies and native art:

```sh
./out/build/wi-dev/fallout2-ce --wi-inspect-layout=42 # synthetic, no assets
python3 tools/wasteland/stage_template.py /path/to/owned/Fallout2 --name proof-layout
(cd runtime/proof-layout && ../../out/build/wi-dev/fallout2-ce --wi-layout-seed=42)
```

The separate v1 layout planner protects the entrance and existing cave routes,
keeps the remaining local cells connected, and leaves space for three loot
objects. Runtime duplicates only a verified scriptless, single-hex blocking
generic scenery/wall prototype already present nearby in the pinned map. It checks visible art,
rechecks native paths after placement and rolls back props/loot on failure.
`wasteland-layout.json` is a diagnostic plan, not a save/checkpoint. The previous
loot/template modes and seed fixtures are retained. For the tested seed-42/Narg
case it adds two native Tree props to the left of entry; visual checks, movement,
cave round trip and save/reload passed. This is a small obstacle cluster, not a
generated room or a MAP writer.

`--wi-probe-layout=42` runs a bounded native Narg initialization and placement
check in a copied runtime, then exits automatically (zero on success). It still
requires SDL and owned assets. Two fresh native probes produced identical layout,
loot and source metadata. Failed interactive layout placement returns to the
menu with an error notice; it does not continue as a completed proof.

The next bounded layout module is a five-tree alcove with one opening, one loot
object inside and two outside. It retains the copied map's floor/roof art:

```sh
./out/build/wi-dev/fallout2-ce --wi-inspect-alcove=42 # synthetic, no assets
python3 tools/wasteland/stage_template.py /path/to/owned/Fallout2 --name proof-alcove
(cd runtime/proof-alcove && ../../out/build/wi-dev/fallout2-ce --wi-alcove-seed=42)
# Or use --wi-probe-alcove=42 for native checks followed by automatic exit.
```

Its independent `grove-alcove.v1` planner samples at most 127 local cells and
emits `wasteland-alcove.json`; the original location and partition contracts,
generators and fixtures remain intact. It protects native exit routes, requires
one reachable opening and three distinct accessible loot cells, and uses the
same verified prop creation/rollback checks. Two fresh seed-42/Narg native probes
match byte-for-byte and preserve all three reachable cave exits. The user confirmed
the requested alcove visibility/pickup, cave round trip and save/reload checks
passed; logs corroborate native transitions, all save/load handlers and normal exit.
This validates the bounded seed-42/Narg case with the pinned assets.
The Director remains unchanged.

The bounded encounter proof adds one seeded ant to the same alcove:

```sh
./out/build/wi-dev/fallout2-ce --wi-inspect-encounter=42 # synthetic, no assets
python3 tools/wasteland/stage_template.py /path/to/owned/Fallout2 --name proof-encounter
(cd runtime/proof-encounter && ../../out/build/wi-dev/fallout2-ce --wi-encounter-seed=42)
# --wi-probe-encounter=42 checks native placement and then exits automatically.
```

`alcove-guard.v1` keeps the ant at least four hexes from entry, off the opening,
loot and protected exit routes. It resolves a checked native ant prototype,
initializes normal native health/AP/team/AI, and removes only its freshly
allocated script before that script can run. Initiate combat to test its native
AI. `wasteland-encounter.json` is a placement diagnostic, not a campaign save.
The seed-42/Narg check passed native combat, live/dead save persistence and cave
travel; see the validation record for scope and preserved evidence.

Source: https://github.com/fallout2-ce/fallout2-ce.git at
`6de8078f02884db8d006bfff490ba05f3af5aabc` (tag `wi-upstream-baseline`).
The original checkout failed to build after an upstream MapElevation change;
a separate commit fixes one local variable's type. See
[baseline evidence](docs/wasteland/BASELINE.md),
[engine integration audit](ENGINE_INTEGRATION.md),
[architecture](ARCHITECTURE.md), [roadmap/test gaps](ROADMAP.md), [session validation](docs/wasteland/VALIDATION.md), and
[asset handling](assets/README.md).

Layout adaptation: this pre-existing engine/ Git checkout remains intact;
project modules live inside it beside src/ to fit the writable workspace.
Wasteland C++ sources are isolated in src/wasteland; upstream README follows.

---

# Fallout 2: Community Engine

Fallout 2:CE is a fully working re-implementation of the classic Fallout engine, optimized for a hassle-free experience on multiple platforms, including Windows, Mac, iOS, Android, and Linux.  It provides high resolution support, quality-of-life improvements, and dozens upon dozens of bug fixes.

This is a fork of the original Fallout 2:CE project, which is no longer getting regular updates.

Popular total conversion mods are supported:
 * [Fallout Et Tu](https://github.com/rotators/Fo1in2): Full Support.  Use v2.0+
 * [Fallout: Sonora](https://github.com/NovaRain/Fallout-Sonora-English): Full Support.
 * [Fallout 2 Restoration Project](https://github.com/BGforgeNet/Fallout2_Restoration_Project): Full Support.
 * Fallout of Nevada: Supported with minor tweaks needed.
 * [Olympus 2207](https://olympus2207.com), Resurrection, Yesterday: partially work but not fully supported.

CE has broad (though not total) compatibility with [Sfall](https://github.com/sfall-team/sfall) scripting extensions.  Many traditional Fallout mods work out of the box, including NPC Armor, Party Orders, FO2Tweaks (we recommend v14.5+), Talking Heads, and many others.

## Installation — [Download Latest CE Release](https://github.com/fallout2-ce/fallout2-ce/releases)

You *must* own the game to play. Purchase your copy on [GOG](https://www.gog.com/game/fallout_2), [Epic Games](https://store.epicgames.com/p/fallout-2) or [Steam](https://store.steampowered.com/app/38410).

Download CE from the link above, then extract the archive into your game folder. Launch `fallout2-ce.exe` on Windows (or the platform-specific executable) to play. CE has a lot of enhancements that require configuration. See [Configuration](#configuration) for examples.

For detailed instructions on how to play on Mac, Linux, iOS, or Android, see the [Platform-specific instructions](#platform-specific-installation).

## Quality of life benefits over vanilla Fallout

* High resolution support
* Party members can loot and barter in place of PC
* Directly equip party members instead of convincing them to use the right equipment.
* Expanded 2-column inventory, loot screens
* Expanded 4-row barter screen
* Expanded AP bar
* Ctrl-click to quickly move items when bartering, looting, or stealing, and auto-balance caps
* Music continues playing between maps
* Auto open doors
* Integrated "HELP" menu
* 44.1 kHz stereo sound/music supported, in .ogg and .wav format as well as legacy .acm
* Last used save slot is remembered
* You can cancel elevator floor selection using ESC
* Spiffy ammo meter with burst-mode segmentation
* Item/Corpse/Container/Critter highlighting (configure using [mods/sfall-mods.ini](https://github.com/sfall-team/sfall/blob/master/artifacts/config_files/sfall-mods.ini))

<table>
  <tr>
    <td align="center">
    <img src="docs/images/two-column-inventory.jpg" alt="Expanded 2-column inventory" width="360"><br>
    <sub>Expanded 2-column inventory</sub>
    </td>
    <td align="center">
      <img src="docs/images/companion-equip.gif" alt="Companion equip screen" width="360"><br>
      <sub>Companion equip screen</sub>
    </td>

  </tr>
  <tr>
    <td align="center">
      <img src="docs/images/party-member-loot.gif" alt="Party member loot" width="360"><br>
      <sub>Party member loot</sub>
    </td>
    <td align="center">
      <img src="docs/images/help-menu.gif" alt="Integrated HELP menu" width="360"><br>
      <sub>Integrated HELP menu</sub>
    </td>
  </tr>
</table>

There are also dozens of small things that just work a little better than they did in the original.  Better pathfinding, fewer graphics glitches, less finicky weapon stacking, and much more.

## Configuration

The main configuration file is `fallout2.cfg`. This contains original game config as well as new CE settings.

Additional settings for screen resolution, UI customization, and map options are now integrated into the main `fallout2.cfg` file (previously part of `f2_res.ini` from Mash's HRP).

Here are some important settings in `fallout2.cfg` under the `[screen]` and `[ui]` sections.  See [the example config](https://github.com/fallout2-ce/fallout2-ce/tree/refs/heads/main/files/fallout2.cfg) for a full list of settings.

```ini
[screen]
resolution_x=1600 ; actual game window size (screen resolution), in pixels
resolution_y=1080
windowed=1 ; 0 = fullscreen
scale=2 ; 1 = original scale, 2 = 2x scale, etc. (e.g. at scale 2 and screen resolution 1920x1080, in-game resolution will be 960x540, thus every pixel is twice as wide and tall)

[ui]
; Maximum number of columns shown in the main inventory and loot/steal windows (valid range: 1..2)
inventory_columns=2

; Set to 1 to expand the barter/trade window vertically, adding a 4th item slot per side (requires ce.dat)
expand_barter_window=1

[qol]
; Allow opening party member inventory from the long-press context menu.
party_trade_from_menu=1

; Allow switching to companions' inventories in loot screens (and barter, in the future)
party_loot_and_barter=1
```

**Recommendations:**
- **Desktops**: Use any size you see fit.
- **Tablets**: Set these values to logical resolution of your device, for example iPad Pro 11 is 1668x2388 (pixels), but its logical resolution is 834x1194 (points).
- **Mobile phones**: Set height to 480, calculate width according to your device screen (aspect) ratio, for example Samsung S21 is 20:9 device, so the width should be 480 * 20 / 9 = 1067.

In time this stuff will receive an in-game interface, right now you have to do it manually. To see all currently working fallout2.cfg settings, just run the game once and quit. It will be automatically updated with defaults for every supported setting.
*Note*: Use of the IFACE_BAR settings requires the `f2_res.dat` file, which contains graphical assets. Various versions are available, but one compatible with the above settings can be found here: [f2_res.dat](https://github.com/fallout2-ce/fallout2-ce/raw/refs/heads/main/files/f2_res.dat)

Note: There are a few settings that are still configured in `ddraw.ini`.  They will be migrated to cfg over time.

## Platform-specific installation

### Windows

[Download](https://github.com/fallout2-ce/fallout2-ce/releases) and unzip into your `Fallout2` folder. Launch the game using `fallout2-ce.exe`.

### Linux

- Use Windows installation as a base - it contains data assets needed to play. Copy `Fallout2` folder somewhere, for example `/home/john/Desktop/Fallout2`.

- Alternatively you can extract the needed files from the GoG installer:

```console
$ sudo apt install innoextract
$ innoextract ~/Downloads/setup_fallout_2_1.02_gog_v1_\(77792\).exe -d Fallout2
```

- Download the Linux release archive, extract `fallout2-ce` and `ce.dat`, and copy them into this folder.

- Run `./fallout2-ce`.

### macOS

> **NOTE**: macOS 10.11 (El Capitan) or higher is required. Runs natively on Intel-based Macs and Apple Silicon.

- Use Windows installation as a base - it contains data assets needed to play. Copy `Fallout2` folder somewhere, for example `/Applications/Fallout2`.

- Alternatively you can use Fallout 2 from Macplay/The Omni Group as a base - you need to extract game assets from the original bundle. Mount CD/DMG, right click `Fallout 2` -> `Show Package Contents`, navigate to `Contents/Resources`. Copy `GameData` folder somewhere, for example `/Applications/Fallout2`.

- Or if you're a Terminal user and have Homebrew installed you can extract the needed files from the GoG installer.  **Note**: You must use "Offline backup game installer", not the main game installer.

```console
$ brew install innoextract
$ innoextract ~/Downloads/setup_fallout_2_1.02_gog_v1_\(77792\).exe -d fallout2
$ mv fallout2 /Applications/Fallout2
```

- [Download](https://github.com/fallout2-ce/fallout2-ce/releases) and copy `Fallout II Community Edition.app` to this folder.

- Run `Fallout II Community Edition.app`.

### Android

> **NOTE**: Fallout 2 was designed with mouse in mind. There are many controls that require precise cursor positioning, which is not possible with fingers. Current control scheme resembles trackpad usage:
- One finger moves mouse cursor around.
- Tap one finger for left mouse click.
- Tap two fingers for right mouse click (switches mouse cursor mode).
- Move two fingers to scroll current view (map view, worldmap view, inventory scrollers).

> **NOTE**: From Android standpoint release and debug builds are different apps. Both apps require their own copy of game assets and have their own savegames. This is intentional. As a gamer just stick with release version and check for updates.

- Download the Android [release](https://github.com/fallout2-ce/fallout2-ce/releases), which contains an .apk file and `ce.dat`.

- Use Windows installation as a base - it contains data assets needed to play. Copy `Fallout2` folder to your device, for example to `Downloads`. You need `master.dat`, `critter.dat`, `patch000.dat`, and `data` folder. Watch for file names - keep (or make) them lowercased (see [Configuration](#configuration)).  Copy `ce.dat` into this folder.

- Copy `fallout2-ce.apk` from the release to your device. Open it with file explorer, follow instructions (install from unknown source).

- When you run the game for the first time it will immediately present file picker. Select the folder from the first step. Wait until this data is copied. A loading dialog will appear, just wait for about 30 seconds. If you're installing total conversion mod or localized version with a large number of unpacked resources in `data` folder it can take up to 20 minutes. Once copied, the game will start automatically.

### iOS

> **NOTE**: See Android note on controls.

- Download `fallout2-ce.ipa`. Use sideloading applications ([AltStore](https://altstore.io/) or [Sideloadly](https://sideloadly.io/)) to install it to your device. Alternatively you can always build from source with your own signing certificate.

- Run the game once. You'll see error message saying "Couldn't find/load text fonts". This step is needed for iOS to expose the game via File Sharing feature.

- Use Finder (macOS Catalina and later) or iTunes (Windows and macOS Mojave or earlier) to copy `master.dat`, `critter.dat`, `patch000.dat`, `ce.dat`, and `data` folder to "Fallout 2" app ([how-to](https://support.apple.com/HT210598)). Watch for file names - keep (or make) them lowercased (see [Configuration](#configuration)).

-

**Controls on iPad:**

- One-finger tap → left click; hold → left button held.
- Two-finger tap → right click (cycles cursor mode between walk and attack).
- One-finger drag → scroll map / windows.
- Two-finger drag → mouse wheel.
- **Three-finger swipe down → ESC** (opens options / main menu).
- **Four-finger long press → F6** (quicksave). Hold four fingers on the screen for about half a second.
- **Three-finger long press → hold Shift** (highlights interactable objects — containers, doors, items — while held).
- Taps on the HUD (interface bar / end-turn / end-combat buttons) act as direct touches so small buttons are easy to hit without aiming.
- A Bluetooth keyboard works for everything else (numbered dialog choices, load game, etc.).

### Browser

> **NOTE**: WebAssembly build with emscripten
```
docker run --rm -v $(pwd):/src emscripten/emsdk:3.1.74 sh -c 'git config --global --add safe.directory "*" && mkdir -p build && cd build && emcmake cmake ../ -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/Emscripten.cmake && emmake make'
```
- Demo available at https://github.com/ololoken/fallout2-ce-ems.git


## Contributing

For build instructions and contributor notes, see [CONTRIBUTING.md](CONTRIBUTING.md).

Integrating Sfall goodies is the top priority. Quality of life updates are OK too.  In any case open up an issue with your suggestion or to notify other people that something is being worked on.

For current sfall compatibility status and the remaining work needed to close gaps, see [SFALL_COMPATIBILITY.md](SFALL_COMPATIBILITY.md).

## For modders

* Robust (though not 100%) Sfall opcode and hook support
* .ogg/.wav support for map music
* .png support for static assets (must be 8bit indexed)
* .zip support for .dat archives
* Working BIS mapper

## License

The source code is this repository is available under the [Sustainable Use License](LICENSE.md).
