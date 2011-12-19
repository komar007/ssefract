#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <dlfcn.h>
#include <SDL/SDL.h>
#include <pthread.h>

#include "fractal_api.h"
#include "io.h"

#define SCR_W 1280
#define SCR_H 1024

int *colors;

int num_colors;

generator_fun_t generate;

void render(SDL_Surface *screen, double complex center, double complex step,
		int x, int y, int fw, int fh)
{
	double complex scr_dim = SCR_W + SCR_H*I;
	double min_x = creal(center) - creal(scr_dim)/2*creal(step);
	double min_y = cimag(center) - cimag(scr_dim)/2*cimag(step);
	double max_x = creal(center) + creal(scr_dim)/2*creal(step);
	double max_y = cimag(center) + cimag(scr_dim)/2*cimag(step);
	fprintf(stderr, "\r%lf %lf %lf %lf", min_x, min_y, max_x, max_y);
	generate(screen->pixels,
			SCR_W, SCR_H,
			x, y, fw, fh,
			min_x, min_y, max_x, max_y,
			50.0, 100,
			num_colors, colors, 0x0);
}

struct params {
	SDL_Surface *screen;
	double complex center;
	double complex step;
	int x; int y; int fw; int fh;
	int *done;
};

void* thread_render(void *data)
{
	struct params *params = data;
	render(params->screen, params->center, params->step,
			params->x, params->y, params->fw, params->fh);
	++(*params->done);
	pthread_exit(NULL);
	return NULL;
}

int done;
struct params params;
pthread_t thread;

void launch_thread(SDL_Surface *screen, double complex center, double complex step,
		int x, int y, int fw, int fh)
{
	struct params *params = (struct params*)malloc(sizeof(struct params));
	params->screen = screen,
	params->center = center,
	params->step = step,
	params->x = x,
	params->y = y,
	params->fw = fw,
	params->fh = fh;
	params->done = &done;
	pthread_create(&thread, NULL, thread_render, (void*)params);
}

double wheel_times = 1.3;

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

int main(int argc, char *argv[])
{
	generate = get_impl_or_die(IMPL_ASM);
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to initialize SDL_VIDEO!");
		exit(-1);
		pthread_exit(NULL);
	}
	SDL_Surface *screen = SDL_SetVideoMode(SCR_W, SCR_H, 32, SDL_SWSURFACE);
	if (screen == NULL) {
		fprintf(stderr, "Unable to initialize screen");
		exit(-1);
		pthread_exit(NULL);
	}
	double complex center = -1.75 -0.04*I;
	num_colors = load_palette("colorpalette.bmp", &colors);
	SDL_Event event;
	double complex step = 0.0002 + 0.0002*I;
	done = 0;
	launch_thread(screen, center, step, 0, 0, SCR_W, SCR_H/2);
	launch_thread(screen, center, step, 0, SCR_H/2, SCR_W, SCR_H/2);
	while (done != 2)
		;
	SDL_UpdateRect(screen, 0, 0, SCR_W, SCR_H);
	bool drag = false;
	while (1) {
		bool redraw = false;
		bool update = false;
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
				case SDLK_ESCAPE:
					exit(0);
					pthread_exit(NULL);
					break;
				}
				break;
			case SDL_QUIT:
				exit(0);
				pthread_exit(NULL);
				break;
			}
		}
		if (redraw) {
			done = 0;
			launch_thread(screen, center, step, 0, 0, SCR_W, SCR_H/2);
			launch_thread(screen, center, step, 0, SCR_H/2, SCR_W, SCR_H/2);
			while (done != 2)
				;
			//render(screen, center, step, 0, 0, SCR_W, SCR_H);
			//blur(screen);
			SDL_UpdateRect(screen, 0, 0, SCR_W, SCR_H);
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
			SDL_BlitSurface(screen, &r_src, screen, &r_dst);
			int frame_w = r_dst.x - r_src.x;
			if (frame_w > 0)
				render(screen, center, step, 0, 0, frame_w, SCR_H);
			else if (frame_w < 0)
				render(screen, center, step, SCR_W+frame_w, 0, -frame_w, SCR_H);
			int frame_h = r_dst.y - r_src.y;
			if (frame_h > 0)
				render(screen, center, step, 0, 0, SCR_W, frame_h);
			else if (frame_h < 0)
				render(screen, center, step, 0, SCR_H+frame_h, SCR_W, -frame_h);
			SDL_UpdateRect(screen, 0, 0, SCR_W, SCR_H);
		} else {
			SDL_Delay(1);
		}
	}
	close_libs_or_die();
	pthread_exit(NULL);
	return 0;
}
