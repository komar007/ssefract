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
#include <errno.h>
#include <dirent.h>

#include "browser.h"

#include "fractal_api.h"
#include "threaded_generator.h"
#include "io.h"
#include "gui.h"

#define SCR_W 1280
#define SCR_H 1024

int *colors[100];
char *palette_names[100];
int num_colors[100];
int cur_palette = 0;
int num_palettes;

int iterations = 100;

generator_fun_t generate;

struct gui_state gui;
struct viewport viewport;

volatile SDL_Surface *large_canvas;

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
	sprintf(gui.status, "center: %.10lf%s%.10lfi | pixel size: %e%s%ei | max iterations: %i",
			creal(center), cimag(center) >= 0 ? " + " : " - ", fabs(cimag(center)),
			creal(psize), cimag(psize) >= 0 ? " + " : " - ", fabs(cimag(psize)),
			iterations);

	threaded_generate(
			generate,
			8,
			surface->pixels, SCR_W, SCR_H,
			x, y, fw, fh,
			min_x, min_y, max_x, max_y,
			50.0, iterations,
			num_colors[cur_palette], colors[cur_palette], 0x0
	);
}

volatile bool thread_ended = false;
volatile bool thread_running = false;

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
			num_colors[cur_palette], colors[cur_palette], 0x0
	);
	FILE *fp = fopen("output.bmp", "w");
	print_bmp(buf, conf->width, conf->height, fp);
	fclose(fp);
	free(buf);
	thread_ended = true;
	thread_running = false;
}

volatile bool nice_valid = true;
volatile bool update_nice = false;

#define SSCALE 4
void render_aa(uint8_t buf[], double complex center, double complex psize,
		int x, int y, int fw, int fh)
{
	double complex scr_dim = SCR_W + SCR_H*I;
	double min_x = creal(center) - creal(scr_dim)/2*creal(psize);
	double min_y = cimag(center) - cimag(scr_dim)/2*cimag(psize);
	double max_x = creal(center) + creal(scr_dim)/2*creal(psize);
	double max_y = cimag(center) + cimag(scr_dim)/2*cimag(psize);
	threaded_generate(
			generate,
			8,
			large_canvas->pixels, SCR_W*SSCALE, SCR_H*SSCALE,
			x*SSCALE, y*SSCALE, fw*SSCALE, fh*SSCALE,
			min_x, min_y, max_x, max_y,
			50.0, iterations,
			num_colors[cur_palette], colors[cur_palette], 0x0
	);
	if (!nice_valid)
		return;
	for (int _y = y; _y < y+fh; ++_y) {
		for (int _x = x; _x < x+fw; ++_x) {
			int r = 0, g = 0, b = 0;
			for (int j = _y*SSCALE; j < _y*SSCALE+SSCALE; ++j) {
				for (int i = _x*SSCALE; i < _x*SSCALE+SSCALE; ++i) {
					r += ((uint8_t*)large_canvas->pixels)[4*(j*SCR_W*SSCALE+i)+0];
					g += ((uint8_t*)large_canvas->pixels)[4*(j*SCR_W*SSCALE+i)+1];
					b += ((uint8_t*)large_canvas->pixels)[4*(j*SCR_W*SSCALE+i)+2];
				}
			}
			r /= SSCALE*SSCALE;
			g /= SSCALE*SSCALE;
			b /= SSCALE*SSCALE;
			buf[4*(_y*SCR_W+_x)+3] = 0x00;
			buf[4*(_y*SCR_W+_x)+0] = r;
			buf[4*(_y*SCR_W+_x)+1] = g;
			buf[4*(_y*SCR_W+_x)+2] = b;
		}
	}
}

volatile bool aa_thread_running = false;
void* render_nice(void *data)
{
	render_aa(gui.canvas->pixels, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
	if (nice_valid)
		update_nice = true;
	aa_thread_running = false;
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
	conf.nthreads = 8;
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
pthread_t aa_thread;
void command_render_nice(char cmd, void *data)
{
	if (aa_thread_running)
		return;
	thread_running = true;
	nice_valid = true;
	update_nice = false;
	aa_thread_running = true;
	pthread_create(&aa_thread, NULL, render_nice, NULL);
	gui_notify(&gui, "generating hi-q image", GREEN, false);
}
void command_palette(char cmd, void *data)
{
	if (cmd == 'p') {
		if (++cur_palette >= num_palettes)
			cur_palette = 0;
	} else {
		if (--cur_palette < 0)
			cur_palette = num_palettes - 1;
	}
	render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
	gui_render(&gui);
	char *txt = malloc(200);
	sprintf(txt, "current color palette: %s", palette_names[cur_palette]);
	gui_notify(&gui, txt, GREEN, false);
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
		.f = &commands_gen_scale},
	{.cmd = 'n', .argtype = NONE,
		.f = &command_render_nice},
	{.cmd = 'p', .argtype = NONE,
		.f = &command_palette},
	{.cmd = 'P', .argtype = NONE,
		.f = &command_palette}
};

int load_colorpalettes(const char* path)
{
	DIR* dir = opendir(path);
	if (!dir)
		return -1;

	struct dirent *file;
	errno = 0;
	int num = 0;
	while ((file = readdir(dir)) != NULL)
	{
		if (!strcmp(file->d_name, "."  ))
			continue;
		if (!strcmp(file->d_name, ".." ))
			continue;

		if (file->d_name[0] == '.')
			continue;

		if (strstr(file->d_name, ".pal")) {
			num_colors[num] = load_palette(file->d_name, &colors[num]);
			palette_names[num] = malloc(strlen(file->d_name) + 1);
			strcpy(palette_names[num], file->d_name);
			++num;
		}
	}
	closedir(dir);
	return num;
}


int main(int argc, char *argv[])
{
	generate = get_impl_or_die(IMPL_ASM);
	gui_init_or_die(&gui, SCR_W, SCR_H, sizeof(commands)/sizeof(*commands), commands);
	viewport.psize = 0.0002 + 0.0002*I;
	viewport.center = -1.75 -0.04*I;

	large_canvas = SDL_CreateRGBSurface(
			SDL_SWSURFACE, SCR_W*SSCALE, SCR_H*SSCALE, 32,
			CMASKS, 0x00000000);

	num_palettes = load_colorpalettes(".");
	render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
	gui_render(&gui);
	SDL_Event event;
	while (1) {
		gui_process_events(&gui);
		if (update_nice) {
			gui_render(&gui);
			update_nice = false;
		}
		if (gui.zoomed) {
			nice_valid = false;
			zoom(gui.zoom_factor, gui.mouse_x, gui.mouse_y);
			render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
			gui_render(&gui);
			gui.zoomed = false;
		} else if (gui.redraw) {
			nice_valid = false;
			render_fract(gui.canvas, viewport.center, viewport.psize, 0, 0, SCR_W, SCR_H);
			gui_render(&gui);
			gui.zoomed = false;
		} else if (gui.dragged) {
			nice_valid = false;
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
