#include <stdio.h>
#include <pthread.h>

#include "threaded_generator.h"

void* work(void *data)
{
	struct job_data *j = data;
	j->generator(
		j->buf,
		j->width, j->height,
		j->frame_x, j->frame_y, j->frame_w, j->frame_h,
		j->min_x, j->min_y, j->max_x, j->max_y,
		j->N, j->iter,
		j->C, j->colors, j->def_color
	);
	pthread_exit(NULL);
	return NULL;
}

void threaded_generate(
		generator_fun_t generator,
		int nthreads,
		int *buf,
		int width, int height,
		double min_x, double min_y, double max_x, double max_y,
		double N, int iter,
		int C, int colors[], int def_color
)
{
	pthread_t threads[nthreads];
	struct job_data params[nthreads];

	for (int i = 0; i < nthreads; ++i) {
		struct job_data tparams = {
			.generator = generator,
			.buf = buf,
			.width = width, .height = height,
			.frame_x = 0, .frame_y = i*(height/nthreads),
			.frame_w = width, .frame_h = height/nthreads,
			.min_x = min_x, .min_y = min_y, .max_x = max_x, .max_y = max_y,
			.N = N, .iter = iter,
			.C = C, .colors = colors, .def_color = def_color
		};
		params[i] = tparams;
		pthread_create(&threads[i], NULL, &work, (void*)&params[i]);
	}
	for (int i = 0; i < nthreads; ++i)
		pthread_join(threads[i], NULL);
}
