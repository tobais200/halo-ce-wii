/*
 * Skinned model vertices. Compressed Halo vertices are weighted onto the
 * model nodes and drawn as triangles. The full shader pass is not linked.
 */
#ifndef WII_MESH_H
#define WII_MESH_H

int wii_mesh_build(long unit_index);
int wii_mesh_triangles(void);
int wii_mesh_triangle(int index, float *xyz);
int wii_mesh_skinned(void);
int wii_mesh_animated(void);
int wii_mesh_weapon(void);
int wii_mesh_shader(void);
int wii_mesh_is_weapon(int index);

#endif
