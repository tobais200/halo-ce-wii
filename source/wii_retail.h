/*
 * Retail map locations the SD loader probes. No FAT calls, so the host
 * check can prove the path list and the missing-map report without a card.
 */
#ifndef WII_RETAIL_H
#define WII_RETAIL_H

#include "wii_stream.h"

int wii_retail_loose_count(void);
char const *wii_retail_loose_dir(int index);
int wii_retail_iso_count(void);
char const *wii_retail_iso_file(int index);
void wii_retail_record(struct wii_stream_stats *stats);

#endif
