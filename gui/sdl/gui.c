#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <dlfcn.h>
#include <SDL/SDL.h>

#define SCR_W 800
#define SCR_H 800


void conv(double x, double y, int *_x, int *_y)
{
	*_x = round((x/2 + .5) * SCR_W);
	*_y = round((y/2 + .5) * SCR_H);
}

void (*generate)(const int*buf, ...) = NULL;

int colors[768];

void render(SDL_Surface *screen, double complex center, double complex step,
		int x, int y, int fw, int fh)
{
	double complex scr_dim = SCR_W + SCR_H*I;
	double min_x = creal(center) - creal(scr_dim)/2*creal(step);
	double min_y = cimag(center) - cimag(scr_dim)/2*cimag(step);
	double max_x = creal(center) + creal(scr_dim)/2*creal(step);
	double max_y = cimag(center) + cimag(scr_dim)/2*cimag(step);
	generate(screen->pixels,
			SCR_W, SCR_H,
			x, y, fw, fh,
			min_x, min_y, max_x, max_y,
			50.0, 50,
			768, colors, 0xffffff);
	SDL_UpdateRect(screen, 0, 0, SCR_W, SCR_H);
}

int main(int argc, char *argv[])
{
	void *lib = dlopen("./fractal.so", RTLD_LAZY);
	generate = dlsym(lib, "generate");
	if (generate == NULL) {
		printf("error loading symbol from fractal.so\n");
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to initialize SDL_VIDEO!");
		exit(-1);
	}
	SDL_Surface *screen = SDL_SetVideoMode(SCR_W, SCR_H, 32, SDL_SWSURFACE);
	if (screen == NULL) {
		fprintf(stderr, "Unable to initialize screen");
		exit(-1);
	}
	double complex center = -1.75 -0.04*I;
	for (int i = 0; i < 768; ++i)
		colors[i] = (i/3) | (((i+1)/3) << 8) | (((i+2)/3) << 16);
	SDL_Event event;
	double complex step = 0.0002 + 0.0002*I;
	render(screen, center, step, 0, 0, SCR_W, SCR_H);
	bool drag = false;
	int diff_x, diff_y;
	while (1) {
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
					step /= 2;
					render(screen, center, step, 0, 0, SCR_W, SCR_H);
					break;
				case SDL_BUTTON_WHEELDOWN:
					step *= 2;
					render(screen, center, step, 0, 0, SCR_W, SCR_H);
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
					diff_x = diff_y = 0;
				}
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_a:
					break;
				case SDLK_z:
					break;
				}
				break;
			case SDL_QUIT:
				exit(0);
				break;
			}
		}
	}
	dlclose(lib);

	return 0;
}
