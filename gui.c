#include "gui.h"

void gui_init(struct gui_state *state, SDL_Surface *screen)
{
	if (TTF_Init() == -1) {
	    printf("Error in TTF_Init: %s\n", TTF_GetError());
	    exit(-1);
	}
	state->font = TTF_OpenFont("mono.ttf", TEXT_HEIGHT);
	if (!state->font) {
		printf("Error opening font: %s\n", TTF_GetError());
		exit(-1);
	}
	state->text = malloc(sizeof(char)*1000);
	state->screen = screen;
}

void gui_free(struct gui_state *state)
{
	TTF_CloseFont(state->font);
	free(state->text);
}

void gui_render(const struct gui_state *state, SDL_Surface *fractal)
{
	SDL_Color fg = {255, 255, 255, 255};
	int bg = 0x00000000;
	SDL_Surface *background = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA,
			state->screen->w, TEXT_HEIGHT+2*MARGIN, 32,
			CMASKS, 0x00000000);
	SDL_SetAlpha(background, SDL_SRCALPHA, 160);
	SDL_Surface *text = TTF_RenderText_Blended(state->font, state->text, fg);
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
	SDL_BlitSurface(text, NULL, state->screen, &font_dest);
	SDL_FreeSurface(text);
	SDL_FreeSurface(background);
}
