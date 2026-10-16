# Wasteland Infinite development

This directory is the existing isolated upstream engine checkout. Keep upstream
layout intact; project modules are siblings of src here because only engine/ is
writable in the session. See upstream.lock.json, LICENSE.md and docs/wasteland/BASELINE.md.
Mark modifications and preserve upstream/third-party notices. No proprietary assets,
original saves, credentials, executable model scripts or hardcoded API keys in Git.

Build: `tools/wasteland/test.sh`. Legacy build: configure with
`-DFALLOUT_VENDORED=OFF -DWASTELAND_INFINITE=OFF`, then build fallout2-ce.
Use the existing CMake targets. Wasteland additions live in src/wasteland.
Do not reorder script indices, map registry entries or save/load handler arrays.
All hooks must default to legacy behavior. No synchronous networking in engine loops.

Phase-1 inspector tests use synthetic geometry, not game assets. Do not call the
prototype playable until it has been visually run with legally owned assets.
Use the isolated runtime staging tool; do not run development mode in an original
installation. Do not broaden the AI Director before runtime placement is validated.
Schema changes need explicit versions, seed replay fixtures and negative tests.
