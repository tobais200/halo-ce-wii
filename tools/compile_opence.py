#!/usr/bin/env python3
"""Compile one OpenCE game/unit/AI translation unit for the Wii or the host."""
import os
import subprocess
import sys

SOURCES = {
    "game.o": "game/game.c",
    "players.o": "game/players.c",
    "cheats.o": "game/cheats.c",
    "game_allegiance.o": "game/game_allegiance.c",
    "weapons.o": "items/weapons.c",
    "projectiles.o": "items/projectiles.c",
    "equipment.o": "items/equipment.c",
    "bipeds.o": "units/bipeds.c",
    "units.o": "units/units.c",
    "vehicles.o": "units/vehicles.c",
    "ai.o": "ai/ai.c",
    "actors.o": "ai/actors.c",
    "encounters.o": "ai/encounters.c",
    "actions.o": "ai/actions.c",
    "path.o": "ai/path.c",
    "props.o": "ai/props.c",
    "ai_communication.o": "ai/ai_communication.c",
    "ai_script.o": "ai/ai_script.c",
    "actor_firing_position.o": "ai/actor_firing_position.c",
    "actor_combat.o": "ai/actor_combat.c",
    "objects.o": "objects/objects.c",
    "object_types.o": "objects/object_types.c",
    "damage.o": "objects/damage.c",
    "models.o": "models/models.c",
    "model_animations.o": "models/model_animations.c",
    "model_animation_definitions.o": "models/model_animation_definitions.c",
    "first_person_weapons.o": "interface/first_person_weapons.c",
    "memory_pool.o": "memory/memory_pool.c",
    "collisions.o": "physics/collisions.c",
    "collision_bsp.o": "physics/collision_bsp.c",
    "collision_models.o": "physics/collision_models.c",
    "collision_features.o": "physics/collision_features.c",
    "bsp2d.o": "physics/bsp2d.c",
    "bsp3d.o": "physics/bsp3d.c",
    "game_engine.o": "game/game_engine.c",
    "sort.o": "cseries/sort.c",
    "devices.o": "devices/devices.c",
    "unit_scripting_commands.o": "units/unit_scripting_commands.c",
    "scenery.o": "objects/scenery.c",
    "recorded_animations.o": "cutscene/recorded_animations.c",
    "camera_scripting.o": "camera/camera_scripting.c",
    "cinematics.o": "cutscene/cinematics.c",
    "hud_messaging.o": "interface/hud_messaging.c",
}

PORT = {
    "wii_collision.o": "port/wii/src/wii_collision.c",
    "wii_collision_host.o": "port/wii/src/wii_collision.c",
    "wii_effects.o": "port/wii/src/wii_effects.c",
    "wii_effects_host.o": "port/wii/src/wii_effects.c",
    "wii_play.o": "port/wii/src/wii_play.c",
    "wii_play_host.o": "port/wii/src/wii_play.c",
    "wii_mesh.o": "port/wii/src/wii_mesh.c",
    "wii_mesh_host.o": "port/wii/src/wii_mesh.c",
}


def includes(root):
    source = os.path.join(root, "third_party", "OpenCE", "source")
    dirs = []
    for dirpath, dirnames, _ in os.walk(source):
        dirs.append(dirpath)
    dirs.append(os.path.join(root, "third_party", "OpenCE", "port", "linux", "game"))
    dirs.append(os.path.join(root, "port", "wii", "hs_shim"))
    dirs.append(os.path.join(root, "port", "wii", "include"))
    dirs.append(os.path.join(root, "source"))
    return dirs


def main():
    if len(sys.argv) != 5:
        sys.stderr.write("usage: compile_opence.py CC ROOT OUTDIR OBJECT\n")
        return 2
    cc, root, outdir, obj = sys.argv[1:]
    rel = SOURCES.get(obj)
    port = PORT.get(obj)
    if rel:
        src = os.path.join(root, "third_party", "OpenCE", "source", rel)
    elif port:
        src = os.path.join(root, port)
        rel = port
    else:
        sys.stderr.write("unknown %s\n" % obj)
        return 2
    out = os.path.join(outdir, obj)
    cmd = cc.split() + [
        "-std=c99", "-O2", "-fshort-wchar", "-fno-strict-aliasing",
        "-DHALO_RELEASE",
        "-Wno-unused", "-Wno-format", "-Wno-incompatible-pointer-types",
        "-include", os.path.join(root, "port", "wii", "hs_shim", "wii_hs_prefix.h"),
        "-include", os.path.join(root, "port", "wii", "include", "wii_sys.h"),
        "-include", os.path.join(root, "third_party", "OpenCE", "port", "linux", "include", "halo_port_limits.h"),
        "-include", os.path.join(root, "third_party", "OpenCE", "port", "linux", "include", "halo_linux_source_fixups.h"),
        "-include", os.path.join(root, "port", "wii", "include", "wii_port_capacity.h"),
    ]
    for directory in includes(root):
        cmd.append("-I" + directory)
    # cinematics.c defines draw_quad. The scoreboard still calls the stub.
    if obj == "cinematics.o":
        cmd.append("-Ddraw_quad=wii_cinematic_quad")
    cmd += ["-c", src, "-o", out]
    print("opence", rel)
    return subprocess.call(cmd)


if __name__ == "__main__":
    sys.exit(main())
