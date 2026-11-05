# Building on Linux with devkitPro

The stub is a normal libogc Wii application. The supported toolchain is **devkitPPC** plus **libogc**, installed with devkitPro pacman. These steps match [devkitPro's Unix instructions](https://devkitpro.org/wiki/Getting_Started) and [devkitPro pacman](https://devkitpro.org/wiki/devkitPro_pacman).

This page is the procedure for the engine stub (`make`, `make dist`). It does not build OpenCE and it does not produce a campaign DOL.

## English

### 1. Install devkitPro pacman (Debian, Ubuntu, and derivatives)

`pkg.devkitpro.org` has to be reachable from the machine. The installer adds the apt source and the `devkitpro-pacman` package. Apt asks for confirmation.

```bash
wget -U "dkp-apt" https://apt.devkitpro.org/install-devkitpro-pacman
chmod +x ./install-devkitpro-pacman
sudo ./install-devkitpro-pacman
```

If apt aborted on the prompt, the repository file is already in place and the package can be finished with:

```bash
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y devkitpro-pacman
```

Other distributions: follow the "Customising Existing Pacman" section on the devkitPro pacman page, then use `pacman` instead of `dkp-pacman`.

### 2. Install the Wii group

```bash
sudo dkp-pacman -Sy
sudo dkp-pacman -S wii-dev
```

`wii-dev` pulls devkitPPC, libogc, libfat, and the Wii examples. A new login reads `/etc/profile.d/devkit-env.sh`. In the current shell:

```bash
source /etc/profile.d/devkit-env.sh
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC="${DEVKITPRO}/devkitPPC"
```

Check the compiler:

```bash
"${DEVKITPPC}/bin/powerpc-eabi-gcc" --version
```

### 3. Build the stub

From the repository root:

```bash
make
make dist
```

`make` writes `boot.elf` and `boot.dol` in the root, with objects in `build/`.

`make dist` rebuilds if needed and packs:

```
dist/apps/halo-ce/boot.dol
dist/apps/halo-ce/boot.elf
dist/apps/halo-ce/meta.xml
dist/apps/halo-ce/icon.png
```

Copy that directory to the SD card as `sd:/apps/halo-ce/`. The Homebrew Channel loads `boot.dol`. The screen must read `Halo CE Wii - engine stub`. HOME exits.

`make clean` removes `build/`, the ELF/DOL, `dist/`, and `port/wii/build/`.

`make` needs `python3`. It rewrites OpenCE fourcc constants into `build/cache_file_formats.c` (the submodule is not edited) and links that file plus `hs_runtime.c`, `memory/data.c`, `game.c`, `players.c`, `units.c`, `bipeds.c`, the weapon files, `ai.c`, `objects.c`, `object_types.c`, `model_animations.c`, `model_animation_definitions.c`, `collisions.c`, `collision_bsp.c`, `game_engine.c`, and `memory_pool.c`. `make host-check` needs `gcc -m32`. It runs the synthetic header, XDVDFS, LZ4, a script-node evaluation of `(set n (+ 1 2))`, a synthetic tag cache (player at 9,8,7, camera height 0.62, one squad location, a 36-round magazine, an identity idle pose, a floor collision BSP, one grenade, and a projectile), and one `game_tick` that must leave a player unit, at least three live objects (player, weapon, actor), a floor hit, a biped collision-model hit, a spawned projectile, projectile damage that lowers vitality, a decoded contrail bitmap, particles that move, a biped step clipped by the structure BSP, a player shot, and an actor shot. `make` also compiles `port/wii/build/wii_platform.o` and does not link it. `make opence` builds this set and exits 0. Bink, networking, and the rasterizer are not compiled. Fetch the submodule first if you want `make opence-status` to report it present:

```bash
git submodule update --init third_party/OpenCE
```

### 4. What a failed environment looks like

`Please set DEVKITPPC` means the variable is empty. Point it at the `devkitPPC` directory, not at `devkitPro` itself.

A 403 from `pkg.devkitpro.org` is Cloudflare refusing the client. The packages are not mirrored in this repository. Fix network access to that host, or install the toolchain on another machine and copy `/opt/devkitpro` across. Do not commit the toolchain.

## Español

### 1. Instalar pacman de devkitPro (Debian, Ubuntu y derivados)

`pkg.devkitpro.org` tiene que ser accesible. El instalador añade el origen apt y el paquete `devkitpro-pacman`. Apt pide confirmación.

```bash
wget -U "dkp-apt" https://apt.devkitpro.org/install-devkitpro-pacman
chmod +x ./install-devkitpro-pacman
sudo ./install-devkitpro-pacman
```

Si apt se detuvo en la pregunta, el repositorio ya está configurado y el paquete se termina así:

```bash
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y devkitpro-pacman
```

Otras distros: sección "Customising Existing Pacman" de la wiki, y luego `pacman` en lugar de `dkp-pacman`.

### 2. Instalar el grupo de Wii

```bash
sudo dkp-pacman -Sy
sudo dkp-pacman -S wii-dev
```

`wii-dev` trae devkitPPC, libogc, libfat y los ejemplos de Wii. Un login nuevo carga `/etc/profile.d/devkit-env.sh`. En la shell actual:

```bash
source /etc/profile.d/devkit-env.sh
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC="${DEVKITPRO}/devkitPPC"
```

Comprueba el compilador:

```bash
"${DEVKITPPC}/bin/powerpc-eabi-gcc" --version
```

### 3. Compilar el stub

En la raíz del repositorio:

```bash
make
make dist
```

`make` deja `boot.elf` y `boot.dol` en la raíz, y los objetos en `build/`.

`make dist` recompila si hace falta y empaqueta:

```
dist/apps/halo-ce/boot.dol
dist/apps/halo-ce/boot.elf
dist/apps/halo-ce/meta.xml
dist/apps/halo-ce/icon.png
```

Copia ese directorio a la SD como `sd:/apps/halo-ce/`. El Homebrew Channel abre `boot.dol`. La pantalla tiene que decir `Halo CE Wii - engine stub`. HOME sale.

`make clean` borra `build/`, el ELF/DOL, `dist/` y `port/wii/build/`.

`make` necesita `python3`. Reescribe las constantes fourcc de OpenCE en `build/cache_file_formats.c` (el submódulo no se edita) y enlaza ese archivo junto con `hs_runtime.c`, `memory/data.c`, `game.c`, `players.c`, `units.c`, `bipeds.c`, las armas, `ai.c`, `objects.c`, `model_animations.c`, `collisions.c`, `game_engine.c` y `memory_pool.c`. `make host-check` necesita `gcc -m32`. Prueba la cabecera sintética, XDVDFS, LZ4, `(set n (+ 1 2))`, una caché de tags sintética (jugador en 9,8,7, altura 0.62, una ubicación de escuadrón, un cargador de 36, un idle identidad, un suelo de colisión, una granada, un proyectil y el modelo de colisión del bípedo) y un `game_tick` que debe dejar al jugador, al menos tres objetos, un impacto en el suelo, un proyectil y daño que baja la vitalidad. `make` también compila `port/wii/build/wii_platform.o` y no lo enlaza. `make opence` construye ese conjunto y sale con 0. Bink, la red y el rasterizador no se compilan. Para que `make opence-status` lo vea:

```bash
git submodule update --init third_party/OpenCE
```

### 4. Si el entorno falla

`Please set DEVKITPPC` significa que la variable está vacía. Tiene que apuntar al directorio `devkitPPC`, no a `devkitPro`.

Un 403 de `pkg.devkitpro.org` es Cloudflare rechazando al cliente. Este repositorio no redistribuye el toolchain. Hay que poder llegar a ese host, o instalarlo en otra máquina y copiar `/opt/devkitpro`. No subas el toolchain al git.
