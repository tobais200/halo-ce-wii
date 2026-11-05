# RAM budget

The Wii has **24 MiB of MEM1 and 64 MiB of MEM2**, 88 MiB in total. That is a hardware ceiling. It is not free space, and this project does not claim more.

## English

### Where bytes go

| Region | What uses it | This bring-up |
| --- | --- | --- |
| MEM1 | DOL, libogc, WPAD/BTE buffers, GX FIFO, framebuffers | Yes. The FIFO stays here (256 KiB, `GX_STUB_FIFO_SIZE`). Two XFBs (console and an off-screen clear). |
| MEM2 | Stream window, LZ4 scratch, map catalog | Yes, and only those. A 1 MiB guard at the top of MEM2 is left unclaimed. |
| Not allocated | Tag cache, texture cache, sound cache | Caps only. Nothing is reserved for them yet. |

Caps, all below the Xbox figures in parentheses:

- Tag data resident: **8 MiB** (Xbox tag cache is 22 MiB at `0x803A6000`, which sits inside Wii MEM1 and does not fit there).
- Textures: **8 MiB** (Xbox 22 MiB, OpenCE desktop 256 MiB).
- Sound: **2 MiB** (Xbox 4 MiB, OpenCE desktop 16 MiB).
- Stream window: **256 KiB**. One chunk. A read larger than this fails.
- Map header passed to OpenCE: **2 KiB** on the stack inside `cache_file_identify`.

`custom_edition_cache_load` is not called. It requires a 23 MiB tag buffer (`CUSTOM_EDITION_TAG_CACHE_BYTES`) and can `malloc` a whole structure BSP. `physical_memory_allocate` is not called either: it uses `XPhysicalAlloc` at fixed Xbox addresses.

### What the screen prints

After boot the console prints the ceiling, then `SYS_GetArena1Size` / `SYS_GetArena2Size` and newlib `mallinfo` for the MEM1 heap, then the MEM2 heap from `__lwp_heap_getinfo`. Those arena numbers are what was left after the DOL and the system reserved their own bytes. They add up to less than 88 MiB. That is expected.

### Link of this tree

Measured with `powerpc-eabi-size` / `powerpc-eabi-readelf` on the `boot.elf` produced here (devkitPPC 16.2.0). Not a hardware run:

| Section | Bytes |
| --- | --- |
| `boot.dol` file | 1687424 |
| `powerpc-eabi-size` text | 1340644 |
| `powerpc-eabi-size` data | 346462 |
| `powerpc-eabi-size` bss | 1798048 |

`.bss` includes libogc (`memb_mem_pool_pbufs` 0x32280, `s_mainStack` 0x20000, `ram_block` 0x10010) and the script VM's fixed buffers: the retail node array (`0x38 + 19001 * 0x14` = 380076 bytes), 64 KiB of script strings, and the script and global tables (512 records each). Thread storage (`0x100 * 0x218`) and the global datum array (`0x400 * 8`) are `malloc`'d when a map's scripts mount, on the MEM1 heap. The 256 KiB stream window is still allocated at runtime from MEM2. `objects_initialize` also `malloc`s the object pool at the Xbox size, 1 MiB (`0x100000`), instead of the desktop 8 MiB. That pool is not in `.bss`. A MEM1 `malloc` of 1 MiB can still fail once the DOL, framebuffers, and script VM are up; this tree was not booted on hardware, so that failure is not measured. The tag working set is a separate block of 23 MiB minus one page from MEM2 (`wii_mem2_alloc`). That is the resident cap. The host check rejects a cap that reaches `0x1700000`. A structure collision BSP gets its own 4 MiB MEM2 arena and is copied only when the whole tree fits. The 23 MiB Xbox tag cache is not copied. `.bss` also holds four actor-debug records (`4 × 0x657c`) so `actor_new` can clear the record for the first spawned actors.

`make host-check` does not boot a Wii. It checks the little-endian fourcc fix, a synthetic XDVDFS image, LZ4, DeLa, that `hs_runtime.c` evaluates `(set n (+ 1 2))` to 3, and that one `game_tick` on a synthetic tag cache (player at 9, 8, 7, one squad location, a 36-round magazine) leaves a player unit and at least three live objects (player, weapon, actor). The caps still fit in 64 MiB.

### Streaming

The menu can draw the first structure BSP with one cache bitmap, and a DeLa screen composited into a 256×256 RGBA buffer in MEM2 (a second buffer holds the bitmap being decoded). Resident BSP geometry stays at 3072 vertices and 4096 triangles. The GX texture is at most 256×256 RGBA8 in MEM1 (`WII_GX_TEXTURE_BYTES`) because the GPU does not read MEM2. Decode follows OpenCE `xbox_textures.c` for bitmap formats 0–3, 6, 8–11, and 14–16. See [SD-LAYOUT.md](SD-LAYOUT.md).

Loose maps: `sd:/halo-ce/maps/*.map`, `sd:/apps/halo-ce/maps`, `sd:/maps`, and the same under `usb:`.

XISO: `sd:/halo-ce/game.iso` (also `usb:/halo-ce/game.iso` and `*.iso` directly inside `sd:/halo-ce` or `usb:/halo-ce`). The Xbox volume descriptor is read at byte `0x10000`. Directories bigger than the 256 KiB window are skipped. Only `.map` headers are identified.

LZ4 (Yann Collet's library, `third_party/lz4`) compresses that 2 KiB header into `sd:/halo-ce/cache/chunks/<name>/<offset>.lz4` and the on-screen lines into `sd:/halo-ce/cache/catalog.lz4`. The next boot can decompress the header instead of reading it again. The rest of the map is not written to the cache and is not loaded.

`fseek` on this devkitPPC is 32-bit. An offset past 2 GiB is rejected and counted on screen as a seek reject. A full DVD image whose `maps` directory lives past that point will not be read until the filesystem layer can seek further. Loose `.map` files are normally far smaller than that.

## Español

El Wii tiene **24 MiB de MEM1 y 64 MiB de MEM2**, 88 MiB en total. Es un techo de hardware, no memoria libre, y este proyecto no afirma más.

MEM1 guarda el DOL, libogc, los búferes de WPAD/BTE, el FIFO de GX (256 KiB) y dos framebuffers. MEM2 guarda la ventana de 256 KiB, el scratch LZ4 y el catálogo. No se reservan la caché de tags, texturas ni sonido: los topes son 8 MiB, 8 MiB y 2 MiB, por debajo de las cifras de Xbox y muy por debajo de las de escritorio de OpenCE.

La pantalla imprime el techo y luego el tamaño real de las arenas. Esas arenas suman menos de 88 MiB porque el DOL y el sistema ya ocuparon lo suyo.

No se llama a `custom_edition_cache_load` ni a `physical_memory_allocate`. El primero pide 23 MiB de tags y puede reservar un BSP entero. El segundo usa direcciones fijas de Xbox dentro de MEM1.

El `boot.dol` de esta compilación pesa 1687424 bytes. El `.bss` es de 1798048 bytes: libogc/BTE más el arreglo fijo de 19001 nodos de script (380076 bytes) y 64 KiB de cadenas. Los hilos de script se piden con `malloc` al montar un mapa. El pool de objetos es otro `malloc`, de 1 MiB (el tamaño de Xbox, no los 8 MiB de escritorio). Cabe en el techo de MEM1 solo si el heap todavía tiene ese bloque; esta compilación no se arrancó en hardware. El conjunto de tags es otro bloque de 23 MiB menos una página en MEM2, que es el tope residente. El BSP de colisión tiene otro bloque de 4 MiB y solo se copia si el árbol cabe entero. La caché de 23 MiB de Xbox no se copia. El `.bss` también guarda cuatro registros de depuración de actores (`4 × 0x657c`) para que `actor_new` pueda limpiar el registro de los primeros actores.

La lectura es por trozos. El menú puede dibujar el primer structure BSP, con tope de 3072 vértices y 4096 triángulos (unos 72 KiB en MEM2). El resto del BSP sigue en la tarjeta. Solo la cabecera de 2 KiB se guarda comprimida con LZ4 en `sd:/halo-ce/cache/`. `fseek` aquí es de 32 bits: un offset por encima de 2 GiB se rechaza.
