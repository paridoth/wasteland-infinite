#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
cmake -S . -B out/build/wi-dev -G Ninja -DCMAKE_BUILD_TYPE=Release -DFALLOUT_VENDORED=OFF -DWASTELAND_INFINITE=ON -DBUILD_TESTING=ON
cmake --build out/build/wi-dev --target fallout2-ce wasteland-tests ce-dat-tool -j "${WI_JOBS:-4}"
ctest --test-dir out/build/wi-dev --output-on-failure
