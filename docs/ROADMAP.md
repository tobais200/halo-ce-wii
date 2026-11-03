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
3. DeLa draws string-list glyphs and follows event-handler flags (open, back, list next/previous, scenario-script name). Other event-function bodies are not linked. A campaign BSP runs OpenCE `game_tick`, which calls the real `players_update_before_game`, `units_update`, `biped_update`, `weapon_update`, `ai_update`, `objects_update`, and `first_person_weapons_update`, plus `hs_runtime.c` (`begin`, `if`, `set`, arithmetic, compares, `sleep`). `player_new` then `player_spawn` places a biped at the first scenario starting location. The biped, model, and animation records that spawn reads are stand-ins (one node, a seat labeled stand, one empty animation, no collision model): the map's tag cache is not fixed up, so `tag_get` cannot return the cyborg tag. The GX orbit still uses `standing_camera_height` from that real biped tag. `ai_update` runs and creates no encounters, because the scenario encounter block is not fixed up. No weapon is placed. Bink, networking, and the OpenGL rasterizer are not compiled.

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
3. DeLa dibuja glifos y sigue las banderas de los manejadores de eventos (abrir, volver, siguiente/anterior, nombre de script). El resto de cuerpos de evento no se enlaza. Un BSP de campaña ejecuta `game_tick` de OpenCE, que llama a las funciones reales de jugador, unidad, bípedo, arma, IA, objetos y arma en primera persona, y a `hs_runtime.c`. `player_spawn` coloca un bípedo en el primer punto de inicio del escenario. La etiqueta de bípedo, el modelo y la animación que lee ese spawn son sustitutos (un nodo, un asiento `stand`, una animación vacía, sin modelo de colisión): la caché de tags del mapa no está reparada, así que `tag_get` no devuelve el tag del cyborg. La órbita GX sigue usando `standing_camera_height` de esa etiqueta bípedo real. `ai_update` corre y no crea encuentros. No se coloca un arma. Bink, la red y el rasterizador OpenGL no se compilan.

### Después

Perfil PowerPC, backend `d3d8_gx`, lectura de mapas que aporte el usuario, audio, mando, y presupuesto de MEM1/MEM2. Hasta entonces no hay campaña jugable.

El port privado de JojoXDK es otro proyecto. Este stub no es ese port.
