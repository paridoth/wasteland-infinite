# Owned game assets

No proprietary data belongs here or in Git. Fallout 2 CE requires a legitimately
owned copy of Fallout 2. Wasteland Infinite does not obtain game data.

`python3 tools/wasteland/stage_runtime.py /path/to/owned/Fallout2` makes real copies
of master.dat, critter.dat, patch archives and loose data into a NEW ignored
`runtime/proof/`. It excludes saves and does not reuse the installation's config.
It never writes to the source installation. Do not point the engine at original
patch/save paths. The launcher currently supports a plain vanilla installation;
conversion mods and Restoration Project are not validated templates.
