/*
 * Stand-in for scenario/scenario.h. The real header includes the sound
 * environment. The script VM needs the trigger-volume cap and the loaded
 * scenario pointer.
 */
#ifndef __SCENARIO_H
#define __SCENARIO_H

enum {
	MAXIMUM_TRIGGER_VOLUMES_PER_SCENARIO = 0x100
};

struct scenario *global_scenario_get(void);
extern long global_scenario_index;

#endif
