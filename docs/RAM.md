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
| `boot.dol` file | 494944 |
| `.text` | 0x5dbe0 (383968) |
| `.rodata` | 0x8706 (34566) |
| `.data` | 0x32c8 (13000) |
| `.bss` | 0x7e20c (516620) |

The large `.bss` objects are libogc, not the map loader: `memb_mem_pool_pbufs` 0x32280 (BTE packet buffers for the WPAD), `s_mainStack` 0x20000, `ram_block` 0x10010. `wii_stream.o` and `wii_mem.o` contribute only a few dozen bytes of `.bss`. The 256 KiB window is allocated at runtime from MEM2, so it is not in the DOL.

`make host-check` does not boot a Wii. It checks the little-endian fourcc fix, a synthetic XDVDFS image, LZ4, and that the caps fit in 64 MiB.

### Streaming

Loose maps: `sd:/halo-ce/maps/*.map`, `sd:/apps/halo-ce/maps`, `sd:/maps`, and the same under `usb:`.

XISO: `sd:/halo-ce/game.iso` (also `usb:/halo-ce/game.iso` and `*.iso` directly inside `sd:/halo-ce` or `usb:/halo-ce`). The Xbox volume descriptor is read at byte `0x10000`. Directories bigger than the 256 KiB window are skipped. Only `.map` headers are identified.

LZ4 (Yann Collet's library, `third_party/lz4`) compresses that 2 KiB header into `sd:/halo-ce/cache/chunks/<name>/<offset>.lz4` and the on-screen lines into `sd:/halo-ce/cache/catalog.lz4`. The next boot can decompress the header instead of reading it again. The rest of the map is not written to the cache and is not loaded.

`fseek` on this devkitPPC is 32-bit. An offset past 2 GiB is rejected and counted on screen as a seek reject. A full DVD image whose `maps` directory lives past that point will not be read until the filesystem layer can seek further. Loose `.map` files are normally far smaller than that.

## Español

El Wii tiene **24 MiB de MEM1 y 64 MiB de MEM2**, 88 MiB en total. Es un techo de hardware, no memoria libre, y este proyecto no afirma más.

MEM1 guarda el DOL, libogc, los búferes de WPAD/BTE, el FIFO de GX (256 KiB) y dos framebuffers. MEM2 guarda la ventana de 256 KiB, el scratch LZ4 y el catálogo. No se reservan la caché de tags, texturas ni sonido: los topes son 8 MiB, 8 MiB y 2 MiB, por debajo de las cifras de Xbox y muy por debajo de las de escritorio de OpenCE.

La pantalla imprime el techo y luego el tamaño real de las arenas. Esas arenas suman menos de 88 MiB porque el DOL y el sistema ya ocuparon lo suyo.

No se llama a `custom_edition_cache_load` ni a `physical_memory_allocate`. El primero pide 23 MiB de tags y puede reservar un BSP entero. El segundo usa direcciones fijas de Xbox dentro de MEM1.

El `boot.dol` de esta compilación pesa 494944 bytes. El `.bss` de unos 516 KiB es sobre todo BTE/libogc (`memb_mem_pool_pbufs`, pila principal), no el cargador de mapas.

La lectura es por trozos. Solo la cabecera de 2 KiB entra en RAM y puede guardarse comprimida con LZ4 en `sd:/halo-ce/cache/`. `fseek` aquí es de 32 bits: un offset por encima de 2 GiB se rechaza.
