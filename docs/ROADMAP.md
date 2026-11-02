# Roadmap

The DOL in this repository is an engine stub. Nothing below is a claim that a campaign runs.

## English

### Done in the stub

- libogc Makefile with `TARGET=boot`, producing `boot.elf` and `boot.dol`.
- VIDEO, GX, and WPAD init. Status screen text: `Halo CE Wii - engine stub`. HOME exits.
- GX clear helpers and a D3D8 → GX note in `source/gx_stub.c` / `source/gx_stub.h`. The desktop OpenCE renderer (`d3d8_gl.c`, OpenGL 4.5) is not usable on GX.
- `make dist` → `dist/apps/halo-ce/` for the Homebrew Channel.
- MIT license, with a note that linking current OpenCE would bring AGPL-3.0-or-later onto the combined work.
- No Halo assets.

### Next, still not a game

1. Submodule [OpenCommunityEdition/OpenCE](https://github.com/OpenCommunityEdition/OpenCE) at `third_party/OpenCE`. That tree is the maintained port of the Xbox build 2342 decompilation (upstream chain: punpckhdq/halo → bnunu/halo-1 → OpenCE). Do not vendor game data with it.
2. Add `port/wii/` as a skeleton: TODOs, a `port.json` that refuses to compile the game, and a Make target that checks the submodule and stops. OpenCE today has `port/linux`, `port/windows`, and `port/android`. It has no Wii port.
3. Keep the stub's `make` / `make dist` independent of that submodule so the Homebrew Channel DOL still builds when OpenCE is absent.

### Later, each one blocked until the previous exists

- A PowerPC profile for OpenCE. The Linux port assumes a host C library, pthreads, SDL3, and OpenGL 4.5 (`port/linux/port.json`).
- `d3d8_gx.c`, a sibling of `d3d8_gl.c`, covering the calls listed in `source/gx_stub.h`. TEV is not a pixel shader. Multi-weight skinning stays on the CPU.
- File IO for a user-supplied Xbox map directory on SD (libfat). The repository still must not contain those files.
- Audio toward AESND, and input from WPAD / Classic / GameCube pad into the existing controller remap, without copying JojoXDK's unpublished tree.
- MEM1/MEM2 budget. The Xbox has a different memory map and a hard disk cache; the Wii does not.

### Out of scope until those exist

A playable campaign, multiplayer, or any statement that retail Halo CE runs on Wii from this tree.

### Separate project

JojoXDK's Halo Wii port is a private work in progress. It is not a branch of this repository. Do not merge unpublished code from it, and do not describe this stub as that port.

## Español

### Hecho en el stub

- Makefile libogc con `TARGET=boot`, que genera `boot.elf` y `boot.dol`.
- VIDEO, GX y WPAD. Texto de estado: `Halo CE Wii - engine stub`. HOME sale.
- Helpers de clear GX y una nota D3D8 → GX. El renderer de escritorio de OpenCE (`d3d8_gl.c`, OpenGL 4.5) no sirve en GX.
- `make dist` → `dist/apps/halo-ce/` para el Homebrew Channel.
- Licencia MIT, con una nota: enlazar el OpenCE actual arrastra AGPL-3.0-or-later a la obra combinada.
- Sin assets de Halo.

### Siguiente, y sigue sin ser un juego

1. Submódulo de OpenCE en `third_party/OpenCE`.
2. Esqueleto `port/wii/` con TODOs y un target de Make que no compila la campaña.
3. Dejar `make` y `make dist` del stub independientes del submódulo.

### Después

Perfil PowerPC, backend `d3d8_gx`, lectura de mapas que aporte el usuario, audio, mando, y presupuesto de MEM1/MEM2. Hasta entonces no hay campaña jugable.

El port privado de JojoXDK es otro proyecto. Este stub no es ese port.
