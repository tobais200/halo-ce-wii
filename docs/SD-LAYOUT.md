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

On a BSP, OpenCE `hs_runtime.c` runs the scenario's script-node array at 30 Hz. `begin`, `if`, `set`, arithmetic, compares, `sleep`, and `wake` are the real evaluators. Later calls return 0. The orbit center is the first player starting location when that block is readable. Biped, weapon, and encounter blocks are counted, not simulated. `game.c` is not linked.

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

En el BSP, `hs_runtime.c` de OpenCE ejecuta el arreglo de nodos del escenario a 30 Hz. `begin`, `if`, `set`, la aritmética, las comparaciones, `sleep` y `wake` son los evaluadores reales. El resto de llamadas devuelve 0. La órbita se centra en el primer punto de inicio del jugador. Los bloques de bípedos, armas y encuentros se cuentan y no se simulan. `game.c` no está enlazado.
