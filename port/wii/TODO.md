# Wii port TODOs

`playable_campaign` in `port.json` stays false until every item below is actually done. Checking the box in this file is not that.

The reference port is `third_party/OpenCE/port/linux` (`port.json`, `src/d3d8_gl.c`, `src/sdl_platform.c`). Upstream has no `port/wii`.

## Do not do

- Do not compile `third_party/OpenCE/source` into `boot.dol` by adding it to the top-level `SOURCES`.
- Do not commit Xbox ISOs, cache maps, bitmaps, sound tags, or title updates.
- Do not import JojoXDK's unpublished tree. That WIP is a different project.
- Do not point the GX notes at OpenGL entry points and call it a port. `d3d8_gl.c` needs a GX sibling, not a link of libGL.

## Bring-up

- [ ] PowerPC defines that replace the Linux port's `DEBUG` / `xbox` pair without pulling `<xtl.h>` through the host SDK headers.
- [ ] Case-insensitive path translate (`platform_translate_path`) onto an SD directory the player chooses. libfat. Return a clear error when the maps directory is absent.
- [ ] MEM1 / MEM2 split. The Linux port's `xbox_memory.c` is the behavior to re-read, not to compile unchanged.
- [ ] `d3d8_gx.c` covering the table in `source/gx_stub.h`. Start from Clear, viewport, textures (tiled GX formats), and Present. TEV comes after a single untextured draw works.
- [ ] CPU skinning for multi-influence vertices. GX will not do that in hardware.
- [ ] Audio: the calls `dsound_sdl.c` satisfies, aimed at AESND, after a silent device succeeds.
- [ ] WPAD / Classic Controller / GameCube pad into the controller remap. Buttons only; no claim about motion aiming.
- [ ] Decide the license of the combined binary before shipping anything that links AGPL OpenCE. The stub is MIT on its own. See `LICENSE`.

## Build hooks that exist now

- `make` and `make dist` build the Homebrew Channel stub and compile `port/wii/build/wii_platform.o`. They do not link OpenCE.
- `make opence-status` prints whether the submodule checkout is present.
- `make opence` exits 2 and compiles nothing from `third_party/OpenCE`.
