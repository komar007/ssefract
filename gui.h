#pragma once

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#define MARGIN 2
#define TEXT_HEIGHT 10

enum gui_mode {
	NORMAL,
	COMMAND
};

struct gui_state {
	SDL_Surface *screen;
	TTF_Font *font;
	enum gui_mode mode;
	char *text;
};

void gui_init(struct gui_state *state, SDL_Surface *screen);
void gui_render(const struct gui_state *state, SDL_Surface *screen);
void gui_free(struct gui_state *state);

#define CMASKS 0x000000ff, 0x0000ff00, 0x00ff0000
