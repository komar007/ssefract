#include <stdbool.h>
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dlfcn.h>
#include <time.h>

#include "fractal_api.h"

int main(int argc, char *argv[])
{
	generator_fun_t functions[] = {
		get_impl_or_die(IMPL_ASM),
		get_impl_or_die(IMPL_C)
	};
	int colors[768];
	for (int i = 0; i < 768; ++i)
		colors[i] = (i/3) | (((i+1)/3) << 8) | (((i+2)/3) << 16);
	for (int size = 1; size <= 4096*4096; size *= 2) {
		int width  = sqrt(size),
		    height = sqrt(size);
		int *buf = (int*)malloc(width*height*sizeof(int));
		printf("%i", size);
		for (int i = 0; i < sizeof(functions)/sizeof(*functions); ++i) {
			int n;
			if (size < 80)
				n = 50000;
			else if (size < 2000)
				n = 500;
			else if (size < 30000)
				n = 50;
			else
				n = 1;
			clock_t time_beg = clock();
			for (int j = 0; j < n; ++j) {
				functions[i](
						buf,
						width, height,
						0, 0, width, height,
						-1.80, -0.09, -1.70, 0.01,
						50.0, 50,
						768, colors, 0xffffff
				);
			}
			int time = clock() - time_beg;
			printf(" %lf", (double)time/n/CLOCKS_PER_SEC*1000);
		}
		printf("\n");
		free(buf);
	}
	close_libs_or_die();
	return 0;
}
