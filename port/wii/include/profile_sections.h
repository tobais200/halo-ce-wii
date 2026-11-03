/*
 * Same macros as OpenCE port/linux/include/profile_sections.h.
 * Kept here so the Wii compile does not pick up that directory's wchar.h.
 */
#ifndef __PROFILE_SECTIONS_H
#define __PROFILE_SECTIONS_H

#define PROFILE_SECTION(variable, name)
#define profile_scope_enter(variable)
#define profile_scope_exit(variable)
#define profile_scope(variable, statement) statement

#endif
