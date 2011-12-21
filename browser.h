#pragma once

#include <complex.h>
#include <stdbool.h>

struct viewport {
	/* complex point of fractal which lays under the center of the
	 * viewport */
	double complex center;
	/* pixel size as complex number */
	double complex psize;
};

struct mark {
	bool used;
	struct viewport viewport;
};

struct render_config {
	int nthreads;
	int width;
	int height;
	double min_x, min_y, max_x, max_y;
	double N;
	int iter;
};
