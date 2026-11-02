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

If `ui` is one of those caches, the DOL opens it and draws the DeLa widget `ui\shell\main_menu\main_menu` (or the first DeLa tag): its background bitmap and child bitmaps, from the tag. D-pad moves a focus box, A follows the child widget or a child named `a10`–`d40` into that scenario's BSP, and B returns to the file list. Event functions, fonts, and string lists are not executed. The file list is only the bootstrap that opens a cache. A on any other map still draws its first structure BSP. Scripts, AI, and weapons are not linked.

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

Si `ui` está en la tarjeta, el DOL lo abre y dibuja el widget DeLa `ui\shell\main_menu\main_menu` (o el primer tag DeLa): el bitmap de fondo y los de los hijos. La cruceta mueve el foco, A entra en el hijo o en un escenario `a10`–`d40`, y B vuelve a la lista de archivos. Las funciones de evento, las fuentes y las listas de texto no se ejecutan. La lista solo abre el cache. A en otro mapa sigue dibujando su primer BSP. Scripts, IA y armas no están enlazados.
