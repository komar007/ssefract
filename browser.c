#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <dlfcn.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <pthread.h>
#include <signal.h>

#include "browser.h"

#include "fractal_api.h"
#include "threaded_generator.h"
#include "io.h"
#include "gui.h"

#define SCR_W 1280
#define SCR_H 1024

int *colors;

int num_colors;

int iterations = 100;

generator_fun_t generate;

struct gui_state gui;
struct viewport viewport;

int output_scale = 4;

void render_fract(SDL_Surface *surface, double complex center, double complex psize,
		int x, int y, int fw, int fh)
{
	double complex scr_dim = SCR_W + SCR_H*I;
	double min_x = creal(center) - creal(scr_dim)/2*creal(psize);
	double min_y = cimag(center) - cimag(scr_dim)/2*cimag(psize);
	double max_x = creal(center) + creal(scr_dim)/2*creal(psize);
	double max_y = cimag(center) + cimag(scr_dim)/2*cimag(psize);
	//fprintf(stderr, "\r%.20lf %.20lf %.20lf %.20lf", min_x, min_y, max_x, max_y);
	sprintf(gui.status, "center: %lf%s%lfi   pixel size: %lf%s%lfi   max iterations: %i",
			creal(center), cimag(center) >= 0 ? "+" : "", cimag(center),
			creal(psize), cimag(psize) >= 0 ? "+" : "", cimag(psize),
			iterations);

	threaded_generate(
			generate,
			2,
			surface->pixels, SCR_W, SCR_H,
			x, y, fw, fh,
			min_x, min_y, max_x, max_y,
			50.0, iterations,
			num_colors, colors, 0x0
	);
}

bool thread_ended = false;
bool thread_running = false;

void* render_to_file(void *data)
{
	struct render_config *conf = data;
	int *buf = malloc(sizeof(int)*conf->width*conf->height);
	threaded_generate(
			generate,
			conf->nthreads,
			buf, conf->width, conf->height,
			0, 0, conf->width, conf->height,
			conf->min_x, conf->min_y, conf->max_x, conf->max_y,
			conf->N, conf->iter,
			num_colors, colors, 0x0
	);
	FILE *fp = fopen("output.bmp", "w");
	print_bmp(buf, conf->width, conf->height, fp);
	fclose(fp);
	free(buf);
	thread_ended = true;
	thread_running = false;
}

void patch_fractal(int diff_x, int diff_y)
{
	viewport.center -= (diff_x)*creal(viewport.psize) + (diff_y)*cimag(viewport.psize)*I;
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
}

void zoom(double zoom_factor, int mx, int my)
{
	viewport.psize *= zoom_factor;
	double complex psize = viewport.psize;
	double ratio = 1 / zoom_factor;
	int w = gui.screen->w,
	    h = gui.screen->h;
	viewport.center -= ((mx-w/2)*creal(psize) + (my-h/2)*cimag(psize)*I)*(1 - ratio);
}

void do_exit(int status)
{
	pthread_exit(NULL);
	exit(-1);
}

struct mark marks[26] = {0};

void command_save_mark(char cmd, char reg, void* data)
{
	marks[reg-'a'].used = true;
	marks[reg-'a'].viewport = viewport;
	char msg[128];
	sprintf(msg, "saved mark in register %c", reg);
	gui_notify(&gui, msg, GREEN, false);
}
void command_goto_mark(char cmd, char reg, void* data)
{
	char msg[128];
	if (marks[reg-'a'].used) {
		viewport = marks[reg-'a'].viewport;
		sprintf(msg, "restored from mark in register %c", reg);
		gui_notify(&gui, msg, GREEN, false);
	} else {
		sprintf(msg, "no mark in register %c", reg);
		gui_notify(&gui, msg, RED, false);
	}
	/* force redraw */
	gui.redraw = true;
}
void command_dump_marks(char cmd, void *data)
{
	int ret = dump_marks("marks", sizeof(marks)/sizeof(marks[0]), marks);
	if (ret == -1)
		gui_notify(&gui, "error writing file 'marks'", RED, false);
	else
		gui_notify(&gui, "dumped marks to file 'marks'", GREEN, false);
}
void command_load_marks(char cmd, void *data)
{
	int ret = load_marks("marks", marks);
	if (ret == sizeof(marks)/sizeof(marks[0]))
		gui_notify(&gui, "restored marks from file 'marks'", GREEN, false);
	else
		gui_notify(&gui, "error reading file 'marks'", RED, false);
}
void commands_hjkl(char cmd, void *data)
{
	if (cmd == 'z' || cmd == 'Z') {
		if (cmd == 'z')
			zoom(0.5, gui.screen->w/2, gui.screen->h/2);
		else
			zoom(2, gui.screen->w/2, gui.screen->h/2);
		render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
		gui_render(&gui);
		return;
	}
	int offs_x = (cmd == 'l' ? -1 : cmd == 'h' ? 1 : 0) * gui.screen->w/3,
	    offs_y = (cmd == 'j' ? -1 : cmd == 'k' ? 1 : 0) * gui.screen->h/3;
	patch_fractal(offs_x, offs_y);
	gui_render(&gui);
}
void commands_iter(char cmd, void *data)
{
	if (cmd == 'i') {
		iterations += 10;
		gui_notify(&gui, "increased max iterations by 10", GREEN, false);
	} else {
		iterations -= 10;
		gui_notify(&gui, "decreased max iterations by 10", GREEN, false);
	}
	render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
	gui_render(&gui);
}
void commands_gen_scale(char cmd, void *data)
{
	if (cmd == 's') {
		output_scale += 1;
	} else {
		output_scale -= 1;
	}
	if (output_scale < 1)
		output_scale = 1;
	char msg[128];
	sprintf(msg, "output resolution %ix%i",
			gui.screen->w*output_scale,
			gui.screen->h*output_scale);
	gui_notify(&gui, msg, GREEN, false);
}

pthread_t thread;
void command_generate(char cmd, void *data)
{
	if (thread_running) {
		pthread_kill(thread);
		thread_running = false;
		gui_notify(&gui, "aborted generation of output.bmp", RED, false);
		return;
	}
	static struct render_config conf;
	int width = output_scale*gui.screen->w, height = output_scale*gui.screen->h;
	double complex scr_dim = gui.screen->w + gui.screen->h*I;
	conf.nthreads = 2;
	conf.width = width, conf.height = height;
	conf.min_x = creal(viewport.center) - creal(scr_dim)/2*creal(viewport.psize);
	conf.min_y = cimag(viewport.center) - cimag(scr_dim)/2*cimag(viewport.psize);
	conf.max_x = creal(viewport.center) + creal(scr_dim)/2*creal(viewport.psize);
	conf.max_y = cimag(viewport.center) + cimag(scr_dim)/2*cimag(viewport.psize);
	conf.N = 50;
	conf.iter = iterations;
	thread_ended = false;
	thread_running = true;
	pthread_create(&thread, NULL, render_to_file, &conf);
	gui_notify(&gui, "started generation to file output.bmp", GREEN, false);
}
void command_quit(void *data)
{
	if (thread_running) {
		pthread_kill(thread);
	}
	do_exit(0);
}

struct command commands[] = {
	{.cmd = 'm', .argtype = REGISTER,
		.f = &command_save_mark},
	{.cmd = '\'', .argtype = REGISTER,
		.f = &command_goto_mark},
	{.cmd = 'q', .argtype = NONE,
		.f = &command_quit},
	{.cmd = 'd', .argtype = NONE,
		.f = &command_dump_marks},
	{.cmd = 'r', .argtype = NONE,
		.f = &command_load_marks},
	{.cmd = 'h', .argtype = NONE,
		.f = &commands_hjkl},
	{.cmd = 'j', .argtype = NONE,
		.f = &commands_hjkl},
	{.cmd = 'k', .argtype = NONE,
		.f = &commands_hjkl},
	{.cmd = 'l', .argtype = NONE,
		.f = &commands_hjkl},
	{.cmd = 'z', .argtype = NONE,
		.f = &commands_hjkl},
	{.cmd = 'Z', .argtype = NONE,
		.f = &commands_hjkl},
	{.cmd = 'i', .argtype = NONE,
		.f = &commands_iter},
	{.cmd = 'I', .argtype = NONE,
		.f = &commands_iter},
	{.cmd = 'g', .argtype = NONE,
		.f = &command_generate},
	{.cmd = 's', .argtype = NONE,
		.f = &commands_gen_scale},
	{.cmd = 'S', .argtype = NONE,
		.f = &commands_gen_scale}
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
			zoom(gui.zoom_factor, gui.mouse_x, gui.mouse_y);
			render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
			gui_render(&gui);
			gui.zoomed = false;
		} else if (gui.redraw) {
			render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
			gui_render(&gui);
			gui.zoomed = false;
		} else if (gui.dragged) {
			patch_fractal(gui.diff_x, gui.diff_y);
			gui_render(&gui);
		} else if (gui.exit_requested) {
			do_exit(0);
		} else if (thread_ended) {
			thread_ended = false;
			gui_notify(&gui, "completed generation of output.bmp", GREEN, false);
		} else {
			SDL_Delay(1);
		}
	}
	close_libs_or_die();
	pthread_exit(NULL);
	return 0;
}
