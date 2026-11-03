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

### Started, still not a game

1. Submodule [OpenCommunityEdition/OpenCE](https://github.com/OpenCommunityEdition/OpenCE) at `third_party/OpenCE`. `make` links `cache_file_formats.c`, `memory/data.c`, `hs/hs_runtime.c`, `game/game.c`, `players.c`, `units.c`, `bipeds.c`, the weapon/projectile/equipment files, `ai.c` with encounters, actors, and paths, `objects.c`, `object_types.c`, `damage.c`, `models.c`, `first_person_weapons.c`, and `memory/memory_pool.c`. `make opence` builds that set and exits 0. `collisions.c` and `game_engine.c` are not in the link.
2. `port/wii/` skeleton remains (`"playable_campaign": false`). The bring-up that does run lives in `source/wii_mem.c`, `source/wii_stream.c`, and `source/xdvdfs.c`.
3. DeLa draws string-list glyphs and follows event-handler flags (open, back, list next/previous, scenario-script name). Other event-function bodies are not linked. A campaign BSP runs OpenCE `game_tick`, which calls the real player, unit, biped, weapon, AI, object, and first-person functions, plus `hs_runtime.c`. Opening a map streams a tag working set out of the cache (the 23 MiB Xbox cache at `0x803A6000` is not mapped). `tag_get` returns those copies. Squad starting locations, the actor palette, and the actor variant's unit are resident, and `ai_place` spawns actors from them. Weapon magazines (round counts) and triggers are resident, so a placed weapon is not empty. Each animation keeps up to 64 KiB of default-pose and frame bytes, byte-swapped to native, and `animation_get_node_orientations` samples them. A larger frame stays on the card and that animation's frame count is forced to 0. The weapon-class idle index points at the resident animation. Collision models and structure BSP tags stay on the card. The GX orbit still uses `standing_camera_height` from the biped tag. Bink, networking, and the OpenGL rasterizer are not compiled.

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

### Empezado, y sigue sin ser un juego

1. Submódulo de OpenCE. `make` enlaza `cache_file_formats.c`, `memory/data.c`, `hs/hs_runtime.c`, `game/game.c`, `players.c`, `units.c`, `bipeds.c`, las armas, `ai.c` con encuentros, actores y caminos, `objects.c`, `object_types.c`, `damage.c`, `models.c`, `first_person_weapons.c` y `memory/memory_pool.c`. `make opence` construye ese conjunto y sale con 0. `collisions.c` y `game_engine.c` no entran.
2. El esqueleto `port/wii/` sigue con la campaña en false. El arranque que sí corre está en `source/wii_stream.c`.
3. DeLa dibuja glifos y sigue las banderas de los manejadores de eventos (abrir, volver, siguiente/anterior, nombre de script). El resto de cuerpos de evento no se enlaza. Un BSP de campaña ejecuta `game_tick` de OpenCE. Al abrir un mapa se lee un conjunto de tags (la caché de 23 MiB en `0x803A6000` no se mapea). Las ubicaciones de los escuadrones y la variante del actor están en memoria, y `ai_place` crea actores. Los cargadores y los gatillos del arma también, así que el arma colocada no está vacía. Cada animación guarda hasta 64 KiB de fotogramas, ya en endian nativo, y `animation_get_node_orientations` los lee. Un fotograma más grande se queda en la tarjeta y su cuenta de frames pasa a 0. Las colisiones y los BSP se quedan en la tarjeta. La órbita GX usa `standing_camera_height` del tag bípedo. Bink, la red y el rasterizador OpenGL no se compilan.

### Después

Perfil PowerPC, backend `d3d8_gx`, lectura de mapas que aporte el usuario, audio, mando, y presupuesto de MEM1/MEM2. Hasta entonces no hay campaña jugable.

El port privado de JojoXDK es otro proyecto. Este stub no es ese port.
