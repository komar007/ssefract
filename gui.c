#include "gui.h"
#include <ctype.h>

void gui_init_or_die(struct gui_state *state, int width, int height, int ncmds, struct command *cmds)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to initialize SDL_VIDEO!");
		exit(-1);
	}
	state->screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);
	if (!state->screen) {
		fprintf(stderr, "Unable to initialize screen");
		exit(-1);
	}
	if (TTF_Init() == -1) {
		printf("Error in TTF_Init: %s\n", TTF_GetError());
		exit(-1);
	}
	state->font = TTF_OpenFont(FONT_NAME, TEXT_HEIGHT);
	if (!state->font) {
		printf("Error opening font: %s\n", TTF_GetError());
		exit(-1);
	}
	state->canvas = SDL_CreateRGBSurface(
			SDL_SWSURFACE, width, height, 32,
			CMASKS, 0x00000000);
	state->drag = false;
	state->mode = NORMAL;
	state->status_text = malloc(sizeof(char)*1000);
	state->ncmds = ncmds;
	state->cmds = cmds;
}

void gui_free(struct gui_state *state)
{
	TTF_CloseFont(state->font);
	free(state->status_text);
}

void gui_render(const struct gui_state *state, SDL_Surface *fractal)
{
	SDL_Color fg = {255, 255, 255, 255};
	int bg = 0x00000000;
	SDL_Surface *background = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA,
			state->screen->w, TEXT_HEIGHT+2*MARGIN, 32,
			CMASKS, 0x00000000);
	SDL_SetAlpha(background, SDL_SRCALPHA, 160);
	SDL_Surface *status_text = TTF_RenderText_Blended(state->font, state->status_text, fg);
	SDL_Rect font_dest = {
		MARGIN, state->screen->h-(TEXT_HEIGHT+MARGIN),
		state->screen->w - 2*MARGIN, TEXT_HEIGHT
	};
	SDL_Rect bg_dest = {
		0, state->screen->h-(TEXT_HEIGHT+2*MARGIN),
		state->screen->w, TEXT_HEIGHT+2*MARGIN
	};
	SDL_FillRect(background, NULL, bg);
	memcpy(state->screen->pixels, fractal->pixels,
			state->screen->h*state->screen->pitch);
	SDL_BlitSurface(background, NULL, state->screen, &bg_dest);
	SDL_BlitSurface(status_text, NULL, state->screen, &font_dest);
	SDL_FreeSurface(status_text);
	SDL_FreeSurface(background);
}

void gui_update(struct gui_state *state)
{
	SDL_UpdateRect(state->screen, 0, 0, state->screen->w, state->screen->h);
}

void gui_process_events(struct gui_state *state)
{
	/* clean flags */
	state->diff_x = 0; state->diff_y = 0;
	state->diff_x = state->diff_y = 0;
	state->dragged = false;
	state->zoomed = false;
	state->redraw = false;
	state->exit_requested = false;
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type)
		{
		case SDL_MOUSEBUTTONDOWN:
			switch (event.button.button) {
			case SDL_BUTTON_LEFT:
				state->drag = true;
				break;
			case SDL_BUTTON_WHEELUP:
				state->zoom_factor = 1/WHEEL_TIMES;
				state->zoomed = true;
				break;
			case SDL_BUTTON_WHEELDOWN:
				state->zoom_factor = WHEEL_TIMES;
				state->zoomed = true;
				break;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			switch (event.button.button) {
			case SDL_BUTTON_LEFT:
				state->drag = false;
			}
			break;
		case SDL_MOUSEMOTION:
			state->mouse_x = event.motion.x;
			state->mouse_y = event.motion.y;
			if (event.button.button == SDL_BUTTON_LEFT && state->drag) {
				state->diff_x += event.motion.xrel;
				state->diff_y += event.motion.yrel;
			}
			break;
		case SDL_KEYUP:
			gui_report_key(state, event.key.keysym.sym);
			break;
		case SDL_QUIT:
			state->exit_requested = true;
			break;
		}
	}
	if (state->diff_x != 0 || state->diff_y != 0)
		state->dragged = true;
}

void gui_report_key(struct gui_state *state, char key)
{
	if (state->mode == COMMAND) {
		if (isalpha(key))
			state->cur_cmd->f.rarg(key, NULL);
		state->mode = NORMAL;
	} else if (state->mode == NORMAL) {
		struct command *cmd = NULL;
		for (int i = 0; i < state->ncmds; ++i)
			if (state->cmds[i].cmd == key)
				cmd = &state->cmds[i];
		if (cmd) {
			if (cmd->argtype == NONE) {
				cmd->f.narg(NULL);
			} else if (cmd->argtype == REGISTER) {
				state->cur_cmd = cmd;
				state->mode = COMMAND;
			}
		}
	}
}
