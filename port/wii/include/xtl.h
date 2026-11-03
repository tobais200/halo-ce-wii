/* Xbox XTL types the collision log asks for. No XDK. */
#ifndef WII_XTL_H
#define WII_XTL_H

typedef long long wii_int64;

#ifndef _LARGE_INTEGER_DEFINED
#define _LARGE_INTEGER_DEFINED
typedef union _LARGE_INTEGER {
	struct {
		unsigned long LowPart;
		long HighPart;
	} u;
	long long QuadPart;
} LARGE_INTEGER;
#endif

#endif
