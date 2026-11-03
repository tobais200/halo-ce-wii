/* Types the Xbox compiler provided and this GCC does not. */
#ifndef WII_SYS_H
#define WII_SYS_H

#define __cdecl
#define __stdcall
#define __fastcall

#ifndef __int64
#define __int64 long long
#endif

#ifndef SCHAR_MAX
#define SCHAR_MAX 127
#endif

#define _snprintf snprintf
#define _vsnprintf vsnprintf

typedef void *PEXCEPTION_POINTERS;
typedef struct wii_context CONTEXT;
#ifndef SCHAR_MIN
#define SCHAR_MIN (-128)
#endif
#ifndef UCHAR_MAX
#define UCHAR_MAX 255
#endif

/* File-scope tags so prototypes in headers name the same struct the
   definitions do. MSVC lifts a tag in a prototype to file scope. */
struct scenario;
struct game_variant;
struct game_options;
struct game_variant_options;
struct damage_data;
struct object_datum;
struct object_header_datum;
union real_vector3d;
union real_point3d;
union real_argb_color;
struct real_matrix4x3;
struct scenario_object_datum;
struct object_placement_data;
struct real_orientation;
struct memory_status;
struct scenario_structure_bsp_reference;
struct render_fog;
struct location;
struct model;

#endif
