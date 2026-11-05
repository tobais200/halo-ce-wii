/*
 * Stand-in for interface/terminal.h. The real header includes the text
 * editor and the Xbox input stack. inspect prints through terminal_shows.
 */
#ifndef __TERMINAL_H
#define __TERMINAL_H

enum {
	_terminal_message_serious = 0,
	_terminal_message_important,
	_terminal_message_chatter
};

boolean terminal_shows(short kind);
extern boolean terminal_command_running;

#endif
