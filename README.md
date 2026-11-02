# Halo CE Wii

Unofficial Wii homebrew **engine stub** for a future port of Halo: Combat Evolved. The DOL that `make` produces boots a status screen and returns to the Homebrew Channel when you press HOME. It is not a game.

**Not affiliated** with Microsoft, Xbox Game Studios, Halo Studios, Activision, or Bungie. Halo and Halo: Combat Evolved are trademarks of Microsoft Corporation.

## English

### What this repository is

A libogc application (`TARGET=boot`) that initializes VIDEO, GX, and WPAD, prints `Halo CE Wii - engine stub`, and exits on HOME. `make dist` writes the Homebrew Channel folder `dist/apps/halo-ce/` (`boot.dol`, `boot.elf`, `meta.xml`, `icon.png`).

The icon is a generic placeholder drawn for this project. It is not the Halo logo.

### What this repository is not

- It is **not playable**. No campaign, multiplayer, menu, or retail loop is linked.
- It does **not** contain Halo assets: no ISOs, map caches, bitmaps, models, audio, or title data. Do not commit any of those.
- If a future build reads game data, **you supply your own legally obtained Xbox Halo: Combat Evolved data**. This project does not distribute it and does not help you dump or share it.
- It is **not** JojoXDK's private work in progress. That Wii port is a separate, unpublished effort. Its source is not in this tree, and this repository does not speak for that author. Nothing here is a release of that WIP.

### OpenCE

[OpenCommunityEdition/OpenCE](https://github.com/OpenCommunityEdition/OpenCE) (Halo CE decompilation of Xbox build 2342, ported to Linux, Windows, and Android) is a submodule at `third_party/OpenCE`. It is not compiled into `boot.dol`.

```bash
git submodule update --init third_party/OpenCE
make opence-status   # prints whether the checkout is present
make opence          # exits 2; does not build a campaign
```

`port/wii/` is a skeleton: one object of failing stubs, a `port.json` with `"playable_campaign": false`, and the TODOs. Upstream has no Wii port. OpenCE is currently AGPL-3.0-or-later; this stub is MIT. Linking the two later changes the obligations of the combined work. See [LICENSE](LICENSE) and [docs/ROADMAP.md](docs/ROADMAP.md).

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

Una aplicación libogc (`TARGET=boot`) que inicializa VIDEO, GX y WPAD, muestra `Halo CE Wii - engine stub` y vuelve al loader al pulsar HOME. `make dist` genera la carpeta del Homebrew Channel `dist/apps/halo-ce/` (`boot.dol`, `boot.elf`, `meta.xml`, `icon.png`).

El icono es un marcador genérico de este proyecto. No es el logotipo de Halo.

### Qué no es

- **No es jugable.** No hay campaña, multijugador, menú ni bucle de juego enlazados.
- **No** incluye datos de Halo: ni ISOs, ni mapas, ni bitmaps, ni modelos, ni audio. No los subas al repositorio.
- Si una versión futura lee datos, **tú aportas tu propia copia legal de Halo: Combat Evolved de Xbox**. Este proyecto no los distribuye ni explica cómo extraerlos o compartirlos.
- **No** es el trabajo privado en curso de JojoXDK. Ese port para Wii es otro esfuerzo, no publicado aquí. Su código no está en este árbol y este repositorio no habla por ese autor.

### OpenCE

[OpenCommunityEdition/OpenCE](https://github.com/OpenCommunityEdition/OpenCE) (decompilación de Halo CE, build 2342 de Xbox, con ports a Linux, Windows y Android) es un submódulo en `third_party/OpenCE`. No se compila dentro de `boot.dol`.

```bash
git submodule update --init third_party/OpenCE
make opence-status   # dice si el checkout está
make opence          # sale con código 2; no construye una campaña
```

`port/wii/` es un esqueleto: un objeto con funciones que fallan, un `port.json` con `"playable_campaign": false` y los TODOs. Upstream no tiene port de Wii. OpenCE actual es AGPL-3.0-or-later; este stub es MIT. Enlazarlos más adelante cambia las obligaciones de la obra combinada. Mira [LICENSE](LICENSE) y [docs/ROADMAP.md](docs/ROADMAP.md).

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
