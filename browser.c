#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <dlfcn.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <pthread.h>

#include "fractal_api.h"
#include "threaded_generator.h"
#include "io.h"
#include "gui.h"

#define SCR_W 1024
#define SCR_H 600

int *colors;

int num_colors;

generator_fun_t generate;

struct gui_state gui_state;

void render_fract(SDL_Surface *surface, double complex center, double complex step,
		int x, int y, int fw, int fh)
{
	double complex scr_dim = SCR_W + SCR_H*I;
	double min_x = creal(center) - creal(scr_dim)/2*creal(step);
	double min_y = cimag(center) - cimag(scr_dim)/2*cimag(step);
	double max_x = creal(center) + creal(scr_dim)/2*creal(step);
	double max_y = cimag(center) + cimag(scr_dim)/2*cimag(step);
	//fprintf(stderr, "\r%.20lf %.20lf %.20lf %.20lf", min_x, min_y, max_x, max_y);
	sprintf(gui_state.text, "%.10lf %.10lf %.10lf %.10lf", min_x, min_y, max_x, max_y);
	threaded_generate(
			generate,
			2,
			surface->pixels, SCR_W, SCR_H,
			x, y, fw, fh,
			min_x, min_y, max_x, max_y,
			50.0, 100,
			num_colors, colors, 0x0
	);
}

double wheel_times = 1.3;

#if 0
#define S0(x, y) ((unsigned char*)screen->pixels)[(y)*screen->pitch + 4*(x)]
#define S1(x, y) ((unsigned char*)screen->pixels)[(y)*screen->pitch + 4*(x)+1]
#define S2(x, y) ((unsigned char*)screen->pixels)[(y)*screen->pitch + 4*(x)+2]
void blur(SDL_Surface *screen)
{
	for (int j = 1; j < screen->h - 1; ++j) {
		for (int i = 1; i < screen->w - 1; ++i) {
			S0(i, j) = (S0(i-1, j-1) + 2*S0(i, j-1) +   S0(i+1, j-1) +
				2*S0(i-1, j)   + 4*S0(i, j)   + 2*S0(i+1, j) +
				  S0(i-1, j+1) + 2*S0(i, j+1) +   S0(i+1, j+1))/16;
			S1(i, j) = (S1(i-1, j-1) + 2*S1(i, j-1) +   S1(i+1, j-1) +
				2*S1(i-1, j)   + 4*S1(i, j)   + 2*S1(i+1, j) +
				  S1(i-1, j+1) + 2*S1(i, j+1) +   S1(i+1, j+1))/16;
			S2(i, j) = (S2(i-1, j-1) + 2*S2(i, j-1) +   S2(i+1, j-1) +
				2*S2(i-1, j)   + 4*S2(i, j)   + 2*S2(i+1, j) +
				  S2(i-1, j+1) + 2*S2(i, j+1) +   S2(i+1, j+1))/16;
		}
	}
}
#endif

void do_exit(int status)
{
	pthread_exit(NULL);
	exit(-1);
}
double complex step = 0.0002 + 0.0002*I;
double complex center = -1.75 -0.04*I;

struct {
	bool used;
	double complex step;
	double complex center;
} marks[256] = {0};

bool redraw = false;
bool update = false;

void command_save_mark(char reg, void* data)
{
	marks[reg].used = true;
	marks[reg].step = step;
	marks[reg].center = center;
}

void command_goto_mark(char reg, void* data)
{
	if (marks[reg].used) {
		step = marks[reg].step;
		center = marks[reg].center;
	}
	redraw = true;
}

struct command commands[] = {
	{.cmd = 'm', .argtype = REGISTER,
		.f = &command_save_mark},
	{.cmd = '\'', .argtype = REGISTER,
		.f = &command_goto_mark}
};

int main(int argc, char *argv[])
{
	generate = get_impl_or_die(IMPL_ASM);
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to initialize SDL_VIDEO!");
		do_exit(-1);
	}
	SDL_Surface *screen = SDL_SetVideoMode(SCR_W, SCR_H, 32, SDL_SWSURFACE);
	if (screen == NULL) {
		fprintf(stderr, "Unable to initialize screen");
		do_exit(-1);
	}
	SDL_Surface *fract = SDL_CreateRGBSurface(
			SDL_SWSURFACE, SCR_W, SCR_H, 32,
			CMASKS, 0x00000000);
	gui_init(&gui_state, screen, sizeof(commands)/sizeof(*commands), commands);

	num_colors = load_palette("colorpalette.bmp", &colors);
	render_fract(fract, center, step,0, 0, SCR_W, SCR_H);
	gui_render(&gui_state, fract);
	SDL_UpdateRect(screen, 0, 0, SCR_W, SCR_H);
	bool drag = false;
	SDL_Event event;
	while (1) {
		int diff_x = 0, diff_y = 0;
		while (SDL_PollEvent(&event)) {
			switch (event.type)
			{
			case SDL_MOUSEBUTTONDOWN:
				switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					drag = true;
					diff_x = diff_y = 0;
					break;
				case SDL_BUTTON_WHEELUP:
					step /= wheel_times;
					center += ((event.button.x-SCR_W/2)*creal(step) + (event.button.y-SCR_H/2)*cimag(step)*I)*(wheel_times-1);
					redraw = true;
					break;
				case SDL_BUTTON_WHEELDOWN:
					step *= wheel_times;
					center += ((event.button.x-SCR_W/2)*creal(step) + (event.button.y-SCR_H/2)*cimag(step)*I)*(1/wheel_times-1);
					redraw = true;
					break;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					drag = false;
				}
				break;
			case SDL_MOUSEMOTION:
				if (event.button.button == SDL_BUTTON_LEFT && drag) {
					diff_x += event.motion.xrel;
					diff_y += event.motion.yrel;
					update = true;
				}
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_a:
					break;
				case SDLK_z:
					break;
				case SDLK_q:
					do_exit(0);
					break;
				}
				break;
			case SDL_KEYUP:
				gui_report_key(&gui_state, event.key.keysym.sym);
				break;
			case SDL_QUIT:
				do_exit(0);
				break;
			}
		}
		if (redraw) {
			render_fract(fract, center, step, 0, 0, SCR_W, SCR_H);
			gui_render(&gui_state, fract);
			//blur(screen);
			SDL_UpdateRect(screen, 0, 0, SCR_W, SCR_H);
			redraw = false;
		} else if (update) {
			center -= (diff_x)*creal(step) + (diff_y)*cimag(step)*I;
			SDL_Rect r_src;
			SDL_Rect r_dst;
			if (diff_x < 0) {
				r_src.x = -diff_x;
				r_dst.x = 0;
				r_src.w = r_dst.w = SCR_W + diff_x;
			} else {
				r_src.x = 0;
				r_dst.x = diff_x;
				r_src.w = r_dst.w = SCR_W - diff_x;
			}
			if (diff_y < 0) {
				r_src.y = -diff_y;
				r_dst.y = 0;
				r_src.h = r_dst.h = SCR_H + diff_y;
			} else {
				r_src.y = 0;
				r_dst.y = diff_y;
				r_src.h = r_dst.h = SCR_H - diff_y;
			}
			SDL_BlitSurface(fract, &r_src, fract, &r_dst);
			int frame_w = r_dst.x - r_src.x;
			if (frame_w > 0)
				render_fract(fract, center, step, 0, 0, frame_w, SCR_H);
			else if (frame_w < 0)
				render_fract(fract, center, step, SCR_W+frame_w, 0, -frame_w, SCR_H);
			int frame_h = r_dst.y - r_src.y;
			if (frame_h > 0)
				render_fract(fract, center, step, 0, 0, SCR_W, frame_h);
			else if (frame_h < 0)
				render_fract(fract, center, step, 0, SCR_H+frame_h, SCR_W, -frame_h);
			gui_render(&gui_state, fract);
			SDL_UpdateRect(screen, 0, 0, SCR_W, SCR_H);
			update = false;
		} else {
			SDL_Delay(1);
		}
	}
	close_libs_or_die();
	pthread_exit(NULL);
	return 0;
}
