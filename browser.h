#pragma once

#include <complex.h>

struct viewport {
	/* complex point of fractal which lays under the center of the
	 * viewport */
	double complex center;
	/* pixel size as complex number */
	double complex psize;
};
