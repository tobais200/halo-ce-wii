# third_party

## OpenCE

`OpenCE/` is a git submodule of [OpenCommunityEdition/OpenCE](https://github.com/OpenCommunityEdition/OpenCE), the maintained port of the Halo: Combat Evolved decompilation (Xbox build 2342). The parent repository records one commit. Fetch it with:

```bash
git submodule update --init third_party/OpenCE
```

The gitlink currently pins `899219cc0a14b03bc12f0517a1e9ee4c37158af8` (upstream tag `build-337` at the time it was recorded). Current OpenCE is AGPL-3.0-or-later. Earlier commits of that history were CC0 1.0; do not assume the pinned commit is CC0. Read `third_party/OpenCE/LICENSE` and `third_party/OpenCE/README.md` at the pinned revision.

The submodule is source, port code, and upstream's own port assets (UI art, fonts, word lists). Those files keep the licenses stored next to them. This repository does not add Xbox ISOs, map caches, or other retail game data, and the submodule is not a place to put them.

`make` compiles `port/linux/game/cache_file_formats.c` from this submodule after `tools/le_fourcc.py` rewrites its four-character constants for a little-endian file layout. The generated file is `build/cache_file_formats.c` and is not committed. `boot.dol` calls `cache_file_identify` only. `make opence` exits 2 and does not compile the rest of the tree. See `port/wii/` and `docs/RAM.md`.

## LZ4

`lz4/` is [LZ4](https://github.com/lz4/lz4) 1.10.0 (`lib/lz4.c`, `lib/lz4.h`), BSD 2-Clause, used as the SD chunk codec for map headers. See `LICENSE` in that directory.
