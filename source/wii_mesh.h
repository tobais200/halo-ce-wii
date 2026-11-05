/*
 * Skinned model vertices. Compressed Halo vertices are weighted onto the
 * model nodes and drawn as triangles. The full shader pass is not linked.
 */
#ifndef WII_MESH_H
#define WII_MESH_H

#include <stdint.h>

int wii_mesh_build(long unit_index);
int wii_mesh_triangles(void);
int wii_mesh_triangle(int index, float *xyz);
int wii_mesh_skinned(void);
int wii_mesh_animated(void);
int wii_mesh_weapon(void);
int wii_mesh_shader(void);
int wii_mesh_overlay(void);
int wii_mesh_replacement(void);
int wii_mesh_marker(void);
int wii_mesh_frame(void);
int wii_mesh_third_person(void);
int wii_mesh_shader_count(void);
float wii_mesh_pose_x(void);
float wii_mesh_pose_z(void);
float wii_mesh_weapon_z(void);
int wii_mesh_hand(void);
const uint8_t *wii_mesh_shader_pixels(uint16_t *width, uint16_t *height);
int wii_mesh_is_weapon(int index);

#endif
