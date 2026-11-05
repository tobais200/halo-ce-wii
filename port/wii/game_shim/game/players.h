#ifndef __PLAYERS_H
#define __PLAYERS_H
/* Fields game.c reads. Retail offsets: unit_index is not at 0; this
   stand-in is only for the quitting-player walk in game.c. */
struct player_datum {
	long unit_index;
	long quit_out_of_game_time;
	boolean quit_out_of_game;
};
#endif
