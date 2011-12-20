#pragma once

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#define MARGIN 2
#define TEXT_HEIGHT 10

enum argument_type {
	NONE,
	REGISTER
};

struct command {
	char cmd;
	enum argument_type argtype;
	union {
		/* no-argument commands */
		void (*narg)(void*);
		/* register-argument commands */
		void (*rarg)(char, void*);
	} f;
};

enum gui_mode {
	NORMAL,
	COMMAND
};

struct gui_state {
	SDL_Surface *screen;
	TTF_Font *font;
	enum gui_mode mode;
	char *text;
	int ncmds;
	struct command *cmds;
	struct command *cur_cmd;
	void* cmd_data;
};

void gui_init(struct gui_state *state, SDL_Surface *screen, int ncmds, struct command *cmds);
void gui_render(const struct gui_state *state, SDL_Surface *screen);
void gui_report_key(struct gui_state *state, char key);
void gui_free(struct gui_state *state);

#define CMASKS 0x000000ff, 0x0000ff00, 0x00ff0000
