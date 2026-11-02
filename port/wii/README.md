# port/wii

Scaffolding for a Wii platform layer next to OpenCE. It is not a campaign port.

Upstream OpenCE ships `port/linux`, `port/windows`, and `port/android`. The submodule at `third_party/OpenCE` is that tree, unmodified. This directory is ours. It compiles one object of failing stubs and refuses `make opence`.

`boot.dol` does not link this object yet. The status screen in `source/main.c` is still the whole program.

See [TODO.md](TODO.md) and [port.json](port.json).
