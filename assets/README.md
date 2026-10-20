# Owned game assets

No proprietary data belongs here or in Git. Fallout 2 CE requires a legitimately
owned copy of Fallout 2. Wasteland Infinite does not obtain game data.

`python3 tools/wasteland/stage_runtime.py /path/to/owned/Fallout2` makes real copies
of master.dat, critter.dat, patch archives, loose data and sound/music into a NEW
ignored `runtime/proof/`. It excludes saves and generated automap/world-map caches
and does not reuse the installation's config.
It never writes to the source installation. Do not point the engine at original
patch/save paths. The launcher currently supports a plain vanilla installation;
conversion mods and Restoration Project are not validated templates.

`tools/wasteland/stage_template.py` additionally supports the pinned English
Steam 303136 asset set. It uses native DAT extraction and SHA-256 checks to place
unchanged `artemple.map` and `arcaves.map` copies plus references in a new runtime.
No renamed maps or map writer are used. Provenance, game bytes, audit reports and
saves stay under ignored runtime/. The runtime guard compares active copies to
the references; staging hashes identify the supported source version, rather
than treating a local marker as an authenticity or security boundary.
