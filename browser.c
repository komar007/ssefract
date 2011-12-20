#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <dlfcn.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <pthread.h>

#include "browser.h"

#include "fractal_api.h"
#include "threaded_generator.h"
#include "io.h"
#include "gui.h"

#define SCR_W 1280
#define SCR_H 1024

int *colors;

int num_colors;

generator_fun_t generate;

struct gui_state gui;
struct viewport viewport;

void render_fract(SDL_Surface *surface, double complex center, double complex psize,
		int x, int y, int fw, int fh)
{
	double complex scr_dim = SCR_W + SCR_H*I;
	double min_x = creal(center) - creal(scr_dim)/2*creal(psize);
	double min_y = cimag(center) - cimag(scr_dim)/2*cimag(psize);
	double max_x = creal(center) + creal(scr_dim)/2*creal(psize);
	double max_y = cimag(center) + cimag(scr_dim)/2*cimag(psize);
	//fprintf(stderr, "\r%.20lf %.20lf %.20lf %.20lf", min_x, min_y, max_x, max_y);
	sprintf(gui.status, "center: %lf%s%lfi   pixel size: %lf%s%lfi",
			creal(center), cimag(center) >= 0 ? "+" : "", cimag(center),
			creal(psize), cimag(psize) >= 0 ? "+" : "", cimag(psize));
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

void do_exit(int status)
{
	pthread_exit(NULL);
	exit(-1);
}

struct {
	bool used;
	struct viewport viewport;
} marks[26] = {0};

void command_save_mark(char reg, void* data)
{
	marks[reg-'a'].used = true;
	marks[reg-'a'].viewport = viewport;
	char msg[128];
	sprintf(msg, "saved mark in register %c", reg);
	gui_notify(&gui, msg, GREEN, false);
}
void command_goto_mark(char reg, void* data)
{
	char msg[128];
	if (marks[reg-'a'].used) {
		viewport = marks[reg-'a'].viewport;
		sprintf(msg, "restored from mark in register %c", reg);
	} else {
		sprintf(msg, "no mark in register %c", reg);
	}
	gui_notify(&gui, msg, GREEN, false);
	/* force redraw */
	gui.redraw = true;
}
void command_quit(void *data)
{
	do_exit(0);
}

struct command commands[] = {
	{.cmd = 'm', .argtype = REGISTER,
		.f = &command_save_mark},
	{.cmd = '\'', .argtype = REGISTER,
		.f = &command_goto_mark},
	{.cmd = 'q', .argtype = NONE,
		.f = &command_quit}
};

int main(int argc, char *argv[])
{
	generate = get_impl_or_die(IMPL_ASM);
	gui_init_or_die(&gui, SCR_W, SCR_H, sizeof(commands)/sizeof(*commands), commands);
	viewport.psize = 0.0002 + 0.0002*I;
	viewport.center = -1.75 -0.04*I;

	num_colors = load_palette("colorpalette.bmp", &colors);
	render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
	gui_render(&gui);
	SDL_Event event;
	while (1) {
		gui_process_events(&gui);
		if (gui.zoomed) {
			viewport.psize *= gui.zoom_factor;
			double complex psize = viewport.psize;
			double ratio = 1 / gui.zoom_factor;
			int mx = gui.mouse_x,
			    my = gui.mouse_y;
			int w = gui.screen->w,
			    h = gui.screen->h;
			viewport.center -= ((mx-w/2)*creal(psize) + (my-h/2)*cimag(psize)*I)*(1 - ratio);
			render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
			gui_render(&gui);
			gui.zoomed = false;
		} else if (gui.redraw) {
			render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
			gui_render(&gui);
			gui.zoomed = false;
		} else if (gui.dragged) {
			viewport.center -= (gui.diff_x)*creal(viewport.psize) + (gui.diff_y)*cimag(viewport.psize)*I;
			SDL_Rect r_src;
			SDL_Rect r_dst;
			if (gui.diff_x < 0) {
				r_src.x = -gui.diff_x;
				r_dst.x = 0;
				r_src.w = r_dst.w = SCR_W + gui.diff_x;
			} else {
				r_src.x = 0;
				r_dst.x = gui.diff_x;
				r_src.w = r_dst.w = SCR_W - gui.diff_x;
			}
			if (gui.diff_y < 0) {
				r_src.y = -gui.diff_y;
				r_dst.y = 0;
				r_src.h = r_dst.h = SCR_H + gui.diff_y;
			} else {
				r_src.y = 0;
				r_dst.y = gui.diff_y;
				r_src.h = r_dst.h = SCR_H - gui.diff_y;
			}
			SDL_BlitSurface(gui.canvas, &r_src, gui.canvas, &r_dst);
			int frame_w = r_dst.x - r_src.x;
			if (frame_w > 0)
				render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, frame_w, SCR_H);
			else if (frame_w < 0)
				render_fract(gui.canvas, viewport.center, viewport.psize, SCR_W+frame_w, 0, -frame_w, SCR_H);
			int frame_h = r_dst.y - r_src.y;
			if (frame_h > 0)
				render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, frame_h);
			else if (frame_h < 0)
				render_fract(gui.canvas, viewport.center, viewport.psize, 0, SCR_H+frame_h, SCR_W, -frame_h);
			gui_render(&gui);
		} else if (gui.exit_requested) {
			do_exit(0);
		} else {
			SDL_Delay(1);
		}
	}
	close_libs_or_die();
	pthread_exit(NULL);
	return 0;
}
