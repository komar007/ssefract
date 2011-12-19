#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "fractal_api.h"
#include "io.h"
#include "threaded_generator.h"

generator_fun_t generate;

int main(int argc, char *argv[])
{
	generate = get_impl_or_die(IMPL_ASM);
	if (!generate) {
		printf("dlsym error: %s\n", dlerror());
		return -1;
	}
	if (argc != 13) {
		printf("usage: %s filename.bmp width height min_x min_y max_x max_y bailout_radius iterations palette.bmp set_color nthreads\n", argv[0]);
		return -1;
	}
	const char *filename = argv[1];
	int width  = atoi(argv[2]);
	int height = atoi(argv[3]);
	double min_x = atof(argv[4]);
	double min_y = atof(argv[5]);
	double max_x = atof(argv[6]);
	double max_y = atof(argv[7]);
	double N     = atof(argv[8]);
	int iter     = atof(argv[9]);
	const char *palette_filename = argv[10];
	int set_color;
	sscanf(argv[11], "%x", &set_color);
	int nthreads = atoi(argv[12]);

	int *colors;
	int num_colors = load_palette("colorpalette.bmp", &colors);
	int *buf = malloc(sizeof(int)*width*height);
	threaded_generate(
			generate,
			nthreads,
			buf, width, height,
			0, 0, width, height,
			min_x, min_y, max_x, max_y,
			N, iter,
			num_colors, colors, set_color
	);
	FILE *fp = fopen(filename, "w");
	print_bmp(buf, width, height, fp);
	fclose(fp);
	free(buf);
	free(colors);
	close_libs_or_die();
	return 0;
}
