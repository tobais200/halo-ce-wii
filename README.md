# Halo CE Wii

Unofficial Wii homebrew port of the Halo: Combat Evolved engine. The DOL that `make` produces boots a status screen and returns to the Homebrew Channel when you press HOME. It is not a finished campaign, and this repository does not include a retail map.

**Not affiliated** with Microsoft, Xbox Game Studios, Halo Studios, Activision, or Bungie. Halo and Halo: Combat Evolved are trademarks of Microsoft Corporation.

## English

### What this repository is

A libogc application (`TARGET=boot`) that initializes VIDEO, GX, and WPAD, prints a status screen, and exits on HOME. The menu lists cache files you put on SD or USB. When a retail `.map` is present, A draws that map's first structure BSP (lightmap or `bitm` texels through OpenCE's swizzle and DXT rules, `GX_TF_RGBA8`, at most 128×128, stored in MEM1). OpenCE `game_tick` is linked and runs on the resident tag set. This is not a finished campaign: this tree has no retail map, and the menu widget that draws scenario help is not linked.

The icon is a generic placeholder drawn for this project. It is not the Halo logo.

### What this repository is not

- It is **not a playable campaign**. No retail map ships here. Multiplayer, Bink, Xbox networking, and the desktop GL rasterizer are not linked.
- It does **not** contain Halo assets: no ISOs, map caches, bitmaps, models, audio, or title data. Do not commit any of those.
- Map bytes come from **your own legally obtained Xbox Halo: Combat Evolved data**. See [SD card](#sd-card) below. This project does not distribute that data and does not help you dump or share it.
- It is **not** JojoXDK's private work in progress. That Wii port is a separate, unpublished effort. Its source is not in this tree, and this repository does not speak for that author. Nothing here is a release of that WIP.

### OpenCE

[OpenCommunityEdition/OpenCE](https://github.com/OpenCommunityEdition/OpenCE) is a submodule at `third_party/OpenCE`. `make opence` compiles the linked campaign subset (game tick, units, AI, weapons, HUD, player UI) into `boot.dol`. `make host-check` runs that subset on a synthetic tag set when no retail file is on the machine. Neither command is a finished campaign.

```bash
git submodule update --init third_party/OpenCE
make                 # boot.dol
make host-check      # synthetic header / XDVDFS / LZ4 / campaign subset
make opence          # PowerPC link of the OpenCE subset
make dist            # dist/apps/halo-ce/ for the SD card
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

### SD card

`make dist` writes this tree. Copy it onto the card as `sd:/apps/halo-ce/`. Paths are absolute `sd:/` paths, not a drive letter.

```
sd:/apps/halo-ce/boot.dol
sd:/apps/halo-ce/boot.elf
sd:/apps/halo-ce/meta.xml
sd:/apps/halo-ce/icon.png
sd:/apps/halo-ce/maps/README.txt
sd:/apps/halo-ce/maps/*.map          # your own files; not in this repo
sd:/apps/halo-ce/game.iso            # optional, only if no loose .map
```

Loose `*.map` is preferred. The loader also probes `sd:/halo-ce/maps`, `sd:/maps`, and the same names on `usb:`. `game.iso` is opened only when no loose map was identified. `fseek` on this toolchain is 32-bit, so an image past 2 GiB is reported and not seeked. With no map, the HUD says `No retail map. Put *.map in sd:/apps/halo-ce/maps`. With one, it says a retail map was found. That is still not a finished campaign. More detail: [docs/SD-LAYOUT.md](docs/SD-LAYOUT.md).

### Dolphin

Use absolute `sd:/` paths in the emulated card (the layout above). AESND is linked (`-laesnd`); if DSP HLE is off, the mix stays silent, and the campaign mix is not finished. Leave MMU emulation off. This DOL is libogc homebrew and does not map the Xbox tag address `0x803A6000`.

### Layout

```
Makefile          libogc app, TARGET=boot, `make dist`
source/main.c     VIDEO + GX + WPAD status screen
source/wii_retail.c  SD path list and missing-map report
apps/halo-ce/maps/README.txt  placeholder, no game data
meta.xml          Homebrew Channel descriptor
icon.png          128×48 generic icon
docs/             Linux build notes and roadmap
```

## Español

### Qué es este repositorio

Una aplicación libogc (`TARGET=boot`) que inicializa VIDEO, GX y WPAD, muestra el estado y sale con HOME. El menú lista los cache de la SD o el USB. Con un `.map` de retail, A dibuja el primer structure BSP. `game_tick` de OpenCE está enlazado. No es una campaña terminada: este árbol no trae un mapa de retail, y el widget de ayuda del escenario no está enlazado.

El icono es un marcador genérico de este proyecto. No es el logotipo de Halo.

### Qué no es

- **No es una campaña jugable.** No hay mapa de retail en el repositorio. Multijugador, Bink, la red de Xbox y el rasterizador GL de escritorio no están enlazados.
- **No** incluye datos de Halo: ni ISOs, ni mapas, ni bitmaps, ni modelos, ni audio. No los subas al repositorio.
- Los bytes del mapa salen de **tu propia copia legal de Halo: Combat Evolved de Xbox**. Mira [Tarjeta SD](#tarjeta-sd). Este proyecto no los distribuye ni explica cómo extraerlos o compartirlos.
- **No** es el trabajo privado en curso de JojoXDK. Ese port para Wii es otro esfuerzo, no publicado aquí. Su código no está en este árbol y este repositorio no habla por ese autor.

### OpenCE

[OpenCommunityEdition/OpenCE](https://github.com/OpenCommunityEdition/OpenCE) es un submódulo en `third_party/OpenCE`. `make opence` compila el subconjunto enlazado (tick, unidades, IA, armas, HUD, UI del jugador). `make host-check` lo corre con tags sintéticos cuando no hay un archivo de retail. Ninguno de los dos es una campaña terminada.

```bash
git submodule update --init third_party/OpenCE
make
make host-check
make opence
make dist
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

### Tarjeta SD

```
sd:/apps/halo-ce/boot.dol
sd:/apps/halo-ce/boot.elf
sd:/apps/halo-ce/meta.xml
sd:/apps/halo-ce/icon.png
sd:/apps/halo-ce/maps/README.txt
sd:/apps/halo-ce/maps/*.map
sd:/apps/halo-ce/game.iso
```

El `.map` suelto va primero. `game.iso` solo se abre si no hubo ningún `.map`, y solo si cabe bajo 2 GiB (`fseek` es de 32 bits). Sin mapa, el HUD dice que falta el retail. Con mapa, dice que lo encontró. Sigue sin ser una campaña terminada.

### Dolphin

Rutas absolutas `sd:/`. AESND está enlazado; sin DSP HLE el audio queda en silencio, y la mezcla de campaña no está terminada. Deja la emulación MMU apagada. El DOL no mapea `0x803A6000`.

### Árbol

```
Makefile          aplicación libogc, TARGET=boot, `make dist`
source/main.c     pantalla de estado VIDEO + GX + WPAD
source/gx_stub.c  helpers GX y notas D3D8 → GX
meta.xml          descriptor del Homebrew Channel
icon.png          icono genérico 128×48
docs/             compilación en Linux y hoja de ruta
```
