#pragma once

#include "fractal_api.h"

struct job_data {
	generator_fun_t generator;
	int *buf;
	int width; int height;
	int frame_x; int frame_y; int frame_w; int frame_h;
	double min_x; double min_y; double max_x; double max_y;
	double N; int iter;
	int C; int *colors; int def_color;
};

void threaded_generate(
		generator_fun_t generator,
		int nthreads,
		int *buf,
		int width, int height,
		double min_x, double min_y, double max_x, double max_y,
		double N, int iter,
		int C, int colors[], int def_color
);
