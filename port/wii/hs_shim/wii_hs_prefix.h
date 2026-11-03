/* Strict ISO hides POSIX strnlen() and random(), which cseries.h declares
   itself. M_PI is still required by real_math.h. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* hs_runtime.c calls the MSVC name. The declaration has to be visible
   before that file; the body is in wii_hs_host.c. */
int _stricmp(char const *a, char const *b);
