#ifndef __PROFILE_H
#define __PROFILE_H
struct profile_section {
	char const *name;
	long section_index;
	boolean active;
};
#define profile_enter(section) ((void)0)
#define profile_exit(section) ((void)0)
void profile_tick_start(void);
void profile_tick_end(void);
#endif
