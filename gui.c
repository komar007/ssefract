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
	state->status = malloc(sizeof(char)*1024);
	state->notification = malloc(sizeof(char)*1024);
	state->ncmds = ncmds;
	state->cmds = cmds;
	state->notification_visible = false;
}

void gui_free(struct gui_state *state)
{
	TTF_CloseFont(state->font);
	free(state->status);
}

void gui_notify(struct gui_state *state, const char *msg, int color, bool permanent)
{
	strcpy(state->notification, msg);
	state->notification_visible = true;
	state->notification_color = color;
	state->notification_end = SDL_GetTicks() + (permanent ? 1000000 : NOTIF_TIME);
	gui_render_panel(state);
}

void gui_render_panel(struct gui_state *state)
{
	SDL_Color fg = {255, 255, 255, 255};
	int bg = 0x00000000;
	SDL_Surface *status;
	if (state->notification_visible)
		status = TTF_RenderText_Blended(state->font, state->notification, fg);
	else
		status = TTF_RenderText_Blended(state->font, state->status, fg);
	int bar_h = status->h + MARGIN;
	int bar_y = state->screen->h - bar_h;
	SDL_Surface *background = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA,
			state->screen->w, bar_h, 32,
			CMASKS, 0x00000000);
	SDL_SetAlpha(background, SDL_SRCALPHA, 160);
	SDL_Rect font_dest = {
		MARGIN, bar_y + MARGIN,
		state->screen->w - MARGIN, status->h
	};
	SDL_Rect bg_dest = {
		0, bar_y,
		state->screen->w, bar_h
	};
	SDL_FillRect(background, NULL, bg);
	if (state->notification_visible) {
		SDL_Rect r = { MARGIN, MARGIN, status->w + MARGIN, status->h};
		SDL_FillRect(background, &r, state->notification_color);
	}
	int offset = bar_y*state->screen->pitch;
	memcpy(state->screen->pixels + offset, state->canvas->pixels + offset,
			bar_h*state->screen->pitch);
	SDL_BlitSurface(background, NULL, state->screen, &bg_dest);
	SDL_BlitSurface(status, NULL, state->screen, &font_dest);
	SDL_FreeSurface(status);
	SDL_FreeSurface(background);
	SDL_UpdateRect(state->screen, 0, bar_y, state->screen->w, bar_h);
}

void gui_render(struct gui_state *state)
{
	memcpy(state->screen->pixels, state->canvas->pixels,
			state->screen->h*state->screen->pitch);
	gui_render_panel(state);
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
			if (event.key.keysym.mod & KMOD_SHIFT)
				gui_report_key(state, event.key.keysym.sym - ('a' - 'A'));
			else
				gui_report_key(state, event.key.keysym.sym);
			break;
		case SDL_QUIT:
			state->exit_requested = true;
			break;
		}
	}
	if (state->diff_x != 0 || state->diff_y != 0)
		state->dragged = true;
	if (SDL_GetTicks() > state->notification_end) {
		state->notification_visible = false;
		gui_render_panel(state);
	}

}

void gui_report_key(struct gui_state *state, char key)
{
	if (state->mode == COMMAND) {
		if (isalpha(key))
			state->cur_cmd->f.rarg(state->cur_cmd->cmd, key, NULL);
		else if (key == SDLK_ESCAPE)
			state->notification_end = 0;
		else
			gui_notify(state, "wrong register", RED, false);
		state->mode = NORMAL;
	} else if (state->mode == NORMAL) {
		struct command *cmd = NULL;
		for (int i = 0; i < state->ncmds; ++i)
			if (state->cmds[i].cmd == key)
				cmd = &state->cmds[i];
		if (cmd) {
			if (cmd->argtype == NONE) {
				cmd->f.narg(key, NULL);
			} else if (cmd->argtype == REGISTER) {
				state->cur_cmd = cmd;
				state->mode = COMMAND;
				gui_notify(state, "> arg: register", BLUE, true);
			}
		}
	}
}
