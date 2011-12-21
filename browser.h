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
