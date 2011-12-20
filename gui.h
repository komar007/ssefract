#pragma once

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <complex.h>
#include <stdbool.h>

#define MARGIN 2
#define TEXT_HEIGHT 10

enum argument_type {
	NONE,
	REGISTER
};

enum gui_mode {
	NORMAL,
	COMMAND
};

struct gui_state {
	SDL_Surface *screen;
	SDL_Surface *canvas;
	TTF_Font *font;
	bool drag;
	/* offset while dragging */
	int diff_x, diff_y;
	/* set then zoomed */
	int mouse_x; int mouse_y;
	bool zoomed;
	double zoom_factor;
	bool redraw;
	bool exit_requested;
	/* set when dragged, together with diff_x, diff_y */
	bool dragged;
	enum gui_mode mode;
	char *status_text;
	int ncmds;
	struct command *cmds;
	struct command *cur_cmd;
	void* cmd_data;
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

void gui_init_or_die(struct gui_state *state, int width, int height, int ncmds, struct command *cmds);
void gui_render(const struct gui_state *state, SDL_Surface *screen);
void gui_update(struct gui_state *state);
void gui_report_key(struct gui_state *state, char key);
void gui_process_events(struct gui_state *state);
void gui_free(struct gui_state *state);

#define CMASKS 0x000000ff, 0x0000ff00, 0x00ff0000
#define WHEEL_TIMES 1.3
#define FONT_NAME "mono.ttf"
