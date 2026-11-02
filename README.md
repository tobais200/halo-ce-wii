# Halo CE Wii

Unofficial Wii homebrew **engine stub** for a future port of Halo: Combat Evolved. The DOL that `make` produces boots a status screen and returns to the Homebrew Channel when you press HOME. It is not a game.

**Not affiliated** with Microsoft, Xbox Game Studios, Halo Studios, Activision, or Bungie. Halo and Halo: Combat Evolved are trademarks of Microsoft Corporation.

## English

### What this repository is

A libogc application (`TARGET=boot`) that initializes VIDEO, GX, and WPAD, prints `Halo CE Wii - engine stub`, and exits on HOME. The menu lists cache files you put on SD or USB. A draws that map's first structure BSP. If the BSP names a lightmap bitmap, or the cache has a `bitm` tag, those texels are decoded with OpenCE's swizzle and DXT rules and sampled on GX (`GX_TF_RGBA8`, at most 128×128, stored in MEM1 because the GPU cannot read MEM2). A map with no BSP triangles but a decodable bitmap, such as `ui`, shows that bitmap. The widget VM, campaign tick, AI, and weapons are not linked.

The icon is a generic placeholder drawn for this project. It is not the Halo logo.

### What this repository is not

- It is **not playable**. No campaign, multiplayer, menu, or retail loop is linked.
- It does **not** contain Halo assets: no ISOs, map caches, bitmaps, models, audio, or title data. Do not commit any of those.
- Map bytes come from **your own legally obtained Xbox Halo: Combat Evolved data** on SD or USB (`sd:/halo-ce/maps/*.map` or `sd:/halo-ce/game.iso`). This project does not distribute that data and does not help you dump or share it. Only a 2 KiB header is read into RAM.
- It is **not** JojoXDK's private work in progress. That Wii port is a separate, unpublished effort. Its source is not in this tree, and this repository does not speak for that author. Nothing here is a release of that WIP.

### OpenCE

[OpenCommunityEdition/OpenCE](https://github.com/OpenCommunityEdition/OpenCE) is a submodule at `third_party/OpenCE`. `make` links one file from it, `port/linux/game/cache_file_formats.c`, and calls `cache_file_identify`. That is a header check through a read callback. It is not the game. `make opence` still exits 2 and does not compile the campaign sources.

```bash
git submodule update --init third_party/OpenCE
make                 # boot.dol: status, memory budget, header identify
make host-check      # synthetic header / XDVDFS / LZ4 on the host compiler
make opence          # exits 2; campaign sources stay unbuilt
```

The combined `boot.dol` includes that AGPL file, so the binary is AGPL-3.0-or-later. The Wii sources written here stay MIT until they are combined with it. See [LICENSE](LICENSE), [docs/RAM.md](docs/RAM.md), and [docs/ROADMAP.md](docs/ROADMAP.md).

`port/wii/` is still a skeleton (`"playable_campaign": false`). Upstream OpenCE has no Wii port. GCC multi-character constants are not the little-endian fourccs stored in map files; `tools/le_fourcc.py` rewrites those constants while compiling and does not modify the submodule.

### Build on Linux (devkitPro)

Full notes: [docs/BUILD-LINUX.md](docs/BUILD-LINUX.md).

```bash
wget -U "dkp-apt" https://apt.devkitpro.org/install-devkitpro-pacman
chmod +x ./install-devkitpro-pacman
sudo ./install-devkitpro-pacman
sudo dkp-pacman -Sy
sudo dkp-pacman -S wii-dev

# new login, or:
source /etc/profile.d/devkit-env.sh
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC="${DEVKITPRO}/devkitPPC"

make
make dist
```

Copy `dist/apps/halo-ce/` to `sd:/apps/halo-ce/` and launch it from the Homebrew Channel.

### Layout

```
Makefile          libogc app, TARGET=boot, `make dist`
source/main.c     VIDEO + GX + WPAD status screen
source/gx_stub.c  GX clear helpers and D3D8 → GX notes
meta.xml          Homebrew Channel descriptor
icon.png          128×48 generic icon
docs/             Linux build notes and roadmap
```

## Español

### Qué es este repositorio

Una aplicación libogc (`TARGET=boot`) que inicializa VIDEO, GX y WPAD, muestra `Halo CE Wii - engine stub` y sale con HOME. El menú lista los cache que pongas en la SD o el USB. A dibuja el primer structure BSP. Si el BSP nombra un lightmap, o el cache tiene un tag `bitm`, esos texels se decodifican con las reglas de swizzle y DXT de OpenCE y se muestrean en GX (como máximo 128×128, en MEM1). Un mapa sin triángulos pero con un bitmap, como `ui`, muestra ese bitmap. La máquina de widgets, el tick de la campaña, la IA y las armas no están enlazados.

El icono es un marcador genérico de este proyecto. No es el logotipo de Halo.

### Qué no es

- **No es jugable.** No hay campaña, multijugador, menú ni bucle de juego enlazados.
- **No** incluye datos de Halo: ni ISOs, ni mapas, ni bitmaps, ni modelos, ni audio. No los subas al repositorio.
- Los bytes del mapa salen de **tu propia copia legal de Halo: Combat Evolved de Xbox** en SD o USB (`sd:/halo-ce/maps/*.map` o `sd:/halo-ce/game.iso`). Este proyecto no los distribuye ni explica cómo extraerlos o compartirlos. A RAM solo entra una cabecera de 2 KiB.
- **No** es el trabajo privado en curso de JojoXDK. Ese port para Wii es otro esfuerzo, no publicado aquí. Su código no está en este árbol y este repositorio no habla por ese autor.

### OpenCE

[OpenCommunityEdition/OpenCE](https://github.com/OpenCommunityEdition/OpenCE) es un submódulo en `third_party/OpenCE`. `make` enlaza un solo archivo suyo, `port/linux/game/cache_file_formats.c`, y llama a `cache_file_identify`. Eso comprueba la cabecera mediante un callback de lectura. No es el juego. `make opence` sigue saliendo con código 2 y no compila las fuentes de la campaña.

```bash
git submodule update --init third_party/OpenCE
make                 # boot.dol: estado, presupuesto de RAM, identify de cabecera
make host-check      # cabecera sintética / XDVDFS / LZ4 con el gcc del host
make opence          # sale con código 2; la campaña no se compila
```

El `boot.dol` combinado incluye ese archivo AGPL, así que el binario es AGPL-3.0-or-later. Las fuentes de Wii escritas aquí siguen en MIT hasta que se combinan con él. Mira [LICENSE](LICENSE), [docs/RAM.md](docs/RAM.md) y [docs/ROADMAP.md](docs/ROADMAP.md).

`port/wii/` sigue siendo un esqueleto (`"playable_campaign": false`). Upstream no tiene port de Wii. Las constantes de cuatro caracteres de GCC no coinciden con el little-endian del archivo; `tools/le_fourcc.py` las reescribe al compilar y no modifica el submódulo.

### Compilar en Linux (devkitPro)

Detalle: [docs/BUILD-LINUX.md](docs/BUILD-LINUX.md).

```bash
wget -U "dkp-apt" https://apt.devkitpro.org/install-devkitpro-pacman
chmod +x ./install-devkitpro-pacman
sudo ./install-devkitpro-pacman
sudo dkp-pacman -Sy
sudo dkp-pacman -S wii-dev

source /etc/profile.d/devkit-env.sh
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC="${DEVKITPRO}/devkitPPC"

make
make dist
```

Copia `dist/apps/halo-ce/` a `sd:/apps/halo-ce/` y ábrelo desde el Homebrew Channel.

### Árbol

```
Makefile          aplicación libogc, TARGET=boot, `make dist`
source/main.c     pantalla de estado VIDEO + GX + WPAD
source/gx_stub.c  helpers GX y notas D3D8 → GX
meta.xml          descriptor del Homebrew Channel
icon.png          icono genérico 128×48
docs/             compilación en Linux y hoja de ruta
```
