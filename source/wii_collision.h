/*
 * Structure-BSP collision geometry streamed out of the map. The 23 MiB
 * tag cache stays on the card. A collision BSP is copied only when the
 * whole tree fits in the collision arena.
 */
#ifndef WII_COLLISION_H
#define WII_COLLISION_H

#include <stdint.h>

int wii_collision_load(uint32_t sbsp_va);
void *wii_collision_load_model(uint32_t coll_va);
int wii_collision_model_hits(void *model);
int wii_damage_hurts(long object_index, long damage_definition);
void wii_collision_write_floor(uint8_t *file, uint32_t sbsp_at, uint32_t collide_at, uint32_t plane_at, uint32_t node_at, uint32_t leaf_at, uint32_t ref_at, uint32_t surface_at);
void wii_collision_reset(void);
int wii_collision_ready(void);
/* Ray from (0, 0, 2) straight down. The synthetic floor at z = 0 hits. */
int wii_collision_hits_floor(void);

#endif
