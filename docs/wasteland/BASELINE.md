# Wasteland Infinite baseline — 2026-10-16

Existing isolated checkout: `/home/hermes/Projects/WastelandInfinite/engine`.
Initial working tree was clean. Origin and exact revision are in `upstream.lock.json`.
No new checkout or movement of upstream sources was necessary.

Host: Linux Mint 22.3 (Ubuntu noble), x86-64; GCC/G++ 13.3.0;
CMake 3.28.3; Ninja; Python 3.12.3; system SDL2 2.30.0 and zlib 1.3.
The documented Linux dependencies were already installed. No system packages changed.

Unmodified configure succeeded:

```sh
cmake -S . -B out/build/wi-baseline -G Ninja -DCMAKE_BUILD_TYPE=Release -DFALLOUT_VENDORED=OFF
cmake --build out/build/wi-baseline --target fallout2-ce -j 4
```

The unmodified build FAILED in `partyMemberPlaceMultihex` at the calls to
`_obj_blocking_at` and `objectSetLocation`: the local `int elevation` cannot convert
implicitly to the upstream commit's new `MapElevation` enum. The only baseline
code fix changes that local's type to `MapElevation`, with a modification notice.
The same build then PASSED and linked `out/build/wi-baseline/fallout2-ce`.
Existing upstream format/truncation warnings remain; no permissive compiler flags used.
Build logs are local ignored artifacts in that build directory.

The unmodified upstream revision remains in Git history and is tagged
`wi-upstream-baseline`. The baseline fix is committed separately from the feature.
No licensing notices removed; LICENSE.md and third-party notices remain intact.
Modified distribution notice: this is Wasteland Infinite, a derivative of Fallout
2 Community Engine, for personal/noncommercial use under the upstream license.

No master.dat or critter.dat found in the home directory during the read-only
asset scan. No proprietary data downloaded, copied, or added. Engine gameplay,
graphical placement and original campaign compatibility remain untested without
owned assets. An asset-free CLI is not a playable-prototype validation.
