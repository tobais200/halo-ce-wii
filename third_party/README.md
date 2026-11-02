# third_party

## OpenCE

`OpenCE/` is a git submodule of [OpenCommunityEdition/OpenCE](https://github.com/OpenCommunityEdition/OpenCE), the maintained port of the Halo: Combat Evolved decompilation (Xbox build 2342). The parent repository records one commit. Fetch it with:

```bash
git submodule update --init third_party/OpenCE
```

The gitlink currently pins `899219cc0a14b03bc12f0517a1e9ee4c37158af8` (upstream tag `build-337` at the time it was recorded). Current OpenCE is AGPL-3.0-or-later. Earlier commits of that history were CC0 1.0; do not assume the pinned commit is CC0. Read `third_party/OpenCE/LICENSE` and `third_party/OpenCE/README.md` at the pinned revision.

The submodule is source, port code, and upstream's own port assets (UI art, fonts, word lists). Those files keep the licenses stored next to them. This repository does not add Xbox ISOs, map caches, or other retail game data, and the submodule is not a place to put them.

Nothing under `third_party/OpenCE` is compiled by `make` or `make dist`. `make opence` exits 2. See `port/wii/`.
