#pragma once

#define BASENAME "fractal"
#define SYMNAME  "generate"

typedef void (*generator_fun_t)(
		int *buf,
		int width, int height,
		int frame_x, int frame_y, int frame_w, int frame_h,
		double min_x, double min_y, double max_x, double max_y,
		double N, int iter,
		int C, int colors[], int def_color
);

enum implementation {
	IMPL_ASM,
	IMPL_C,
	COUNT_IMPL,
};

generator_fun_t get_impl_or_die(enum implementation impl);
void close_libs_or_die();
