# SD and USB layout

The repository does not contain an ISO, a cache map, or any other Halo data. The DOL only reads files you already have from your own Xbox copy of Halo: Combat Evolved.

## English

Copy `dist/apps/halo-ce/` to the card as `sd:/apps/halo-ce/` (`boot.dol`, `boot.elf`, `meta.xml`, `icon.png`).

Put the game data in either of these places:

```
sd:/halo-ce/maps/*.map
sd:/halo-ce/game.iso
```

Also searched, and the same names under `usb:`:

```
sd:/apps/halo-ce/maps/*.map
sd:/maps/*.map
usb:/halo-ce/maps/*.map
usb:/maps/*.map
usb:/halo-ce/game.iso
usb:/game.iso
```

`*.iso` directly inside `sd:/halo-ce/` or `usb:/halo-ce/` is scanned too (up to eight images). An `.iso` must be an Xbox XDVDFS image. The loader reads the volume at byte `0x10000`, then `maps/*.map` inside it, and does not copy the image into RAM.

`fseek` and libfat's `f_lseek` on this devkitPPC are 32-bit (`off_t` is signed, FatFs `FSIZE_t` is one PowerPC register). `fseeko64` is declared and not provided by newlib here, so an offset past 2 GiB still fails. Loose `.map` files are the layout that works. A file the FAT layer can describe only up to 4 GiB cannot be seeked further from this DOL.

Opening `ui` draws DeLa bitmaps and, when the widget names a string list and a font, the list's UTF-16 text with that font's glyph pixels. A button that matches a widget event handler follows that handler's flags: open or replace a widget, go back, move a list, or launch a campaign id named by the handler script. Other event functions are reported as unlinked.

On a BSP, OpenCE `game_tick` runs at 30 Hz. It updates scripts through `hs_runtime.c` (`begin`, `if`, `set`, arithmetic, compares, `sleep`, `wake`) and then calls the real player, unit, biped, weapon, AI, object, and first-person weapon functions. Opening the map streams a tag working set (at most 4 MiB in MEM2). `player_spawn` places the biped named by the globals tag at the scenario starting locations. `objects_place` places up to four weapons, with magazine round counts and triggers from the weapon tag. `ai_place` spawns actors from squad starting locations (up to four per squad, eight encounters). Each animation keeps up to 64 KiB of frame bytes, and `animation_get_node_orientations` samples the weapon-class idle. A larger blob stays on the card. The first structure BSP's collision BSP is copied when it fits in 4 MiB, and `collision_test_vector` uses it. Biped collision models (up to 32 nodes) are copied too, and projectile impact damage is applied by `object_cause_damage`. `effect_new` decodes a contrail bitmap up to 128×128. Particles and a 32-point contrail are drawn as GX billboards over the BSP, and a first-person block sits in front of the eye. After each tick the stick walks the biped: the move is clipped by the structure BSP and the feet snap to the floor. A fires the resident projectile. A nearby enemy actor fires back. Hits use `object_cause_damage`. DeLa campaign ids (`a10` through `d40`) open that map. Projectile, equipment, grenade, and major-upgrade tags are copied with the weapon and actor variant. `game_engine.c` runs, and a campaign does not start a multiplayer engine. The orbit center is the first starting location, plus `standing_camera_height` from the biped tag (offset 0x400). Bink, networking, and the rasterizer are not linked.

Optional header cache, written by the DOL, not required:

```
sd:/halo-ce/cache/catalog.lz4
sd:/halo-ce/cache/chunks/<name>/<offset>.lz4
```

Those files are compressed copies of headers and the menu text, not a redistributed game.

Wiimote: D-pad selects, A draws, B returns from the view, HOME exits. Shaders are not sampled. Orbit uses the D-pad while the BSP is on screen.

## Español

El repositorio no lleva ISO, mapas ni otros datos de Halo. El DOL solo lee archivos de tu propia copia de Xbox de Halo: Combat Evolved.

Copia `dist/apps/halo-ce/` a la tarjeta como `sd:/apps/halo-ce/`.

Los datos del juego:

```
sd:/halo-ce/maps/*.map
sd:/halo-ce/game.iso
```

También se buscan `sd:/apps/halo-ce/maps`, `sd:/maps` y las mismas rutas en `usb:`. Un `.iso` tiene que ser una imagen XDVDFS. El offset máximo de lectura es 2 GiB con este toolchain; los `.map` sueltos son la forma fiable.

Si `ui` está en la tarjeta, el DOL lo abre y dibuja el widget DeLa, incluidos los glifos de su lista de texto con la fuente del cache. Un botón con manejador de evento abre el widget, vuelve, mueve la lista o lanza un id de campaña escrito en el script del manejador. Las demás funciones de evento quedan sin enlazar.

En el BSP, `game_tick` de OpenCE corre a 30 Hz. Al abrir el mapa se lee un conjunto de tags (como máximo 1 MiB en MEM2). `player_spawn` coloca el bípedo. `objects_place` coloca hasta cuatro armas con sus cargadores. `ai_place` crea actores desde las ubicaciones de los escuadrones. Cada animación guarda hasta 64 KiB de fotogramas, y `animation_get_node_orientations` lee el idle. Un blob mayor se queda en la tarjeta. El BSP de colisión del structure se copia si cabe en 4 MiB. También se leen proyectiles, equipo, granadas y la mejora mayor. `game_engine.c` corre, y la campaña no enciende el motor multijugador. La órbita usa el primer punto más `standing_camera_height` del tag bípedo (desplazamiento 0x400). Bink, la red y el rasterizador no están enlazados.
