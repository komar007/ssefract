#include <stdbool.h>
#include <complex.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <dlfcn.h>

double maxn;

int (*compute_point)(double complex c, double N, int k, double *abs) = NULL;
void (*generate_ptr)( unsigned char *buf, int width, int height, int frame_x, int frame_y, int frame_w, int frame_h, double N, int iter, double min_x, double min_y, double max_x, double max_y) = NULL;

void generate(
		unsigned char *buf,
		int width, int height,
		int frame_x, int frame_y,
		int frame_w, int frame_h,
		double N, int iter,
		double min_x, double min_y,
		double max_x, double max_y
)
{
	maxn = 0;
	double range_x = max_x - min_x,
	       range_y = max_y - min_y;
	double step_x = range_x / width,
	       step_y = range_y / height;
	buf += frame_y*width + frame_x;
	for (int j = 0; j < frame_h; ++j) {
		for (int i = 0; i < frame_w; ++i) {
			double complex c, z;
			c = (min_x + step_x * (frame_x+i)) + (min_y + step_y * (frame_y+j))*I;
			buf[i] = 255;
#ifdef C
			z = 0 + 0*I;
			for (int k = 0; k < iter; ++k) {
				double abs;
				if ((abs = cabs(z)) > N) {
					double v = k - log2(log2(abs)/log2(N));
					if (v > maxn)
						maxn = v;
					buf[i] = 5*v;
					break;
				}
				z = fabs(creal(z)) + I*fabs(cimag(z));
				z = z*z + c;
			}
#else
			double v;
			int k = compute_point(c, N, iter, &v);
			if (k != 0) {
				if (v > maxn)
					maxn = v;
				buf[i] = 5*v;
			}
#endif
		}
		buf += width;
	}
}

void print_xpm(const unsigned char *buf, int width, int height, FILE *fp)
{
	fprintf(fp, "/* XPM */\n");
	fprintf(fp, "static char *XFACE[] = {");
	fprintf(fp, "\"%i %i %i %i\"\n", width, height, 256, 2);
	for (int i = 0; i < 256; ++i)
		fprintf(fp, "\"%.2x c #%.2x%.2x%.2x\"\n", i, i, i, i);
	for (int j = 0; j < height; ++j) {
		fprintf(fp, "\"");
		for (int i = 0; i < width; ++i)
			fprintf(fp, "%.2x", *buf++);
		fprintf(fp, "\"\n");
	}
}

int main(int argc, char *argv[])
{
	void *lib = dlopen("./fractal.so", RTLD_LAZY);
	compute_point = dlsym(lib, "compute_point");
	if (compute_point == NULL) {
		printf("error loading symbol from fractal.so\n");
	}
	generate_ptr = dlsym(lib, "generate");
	if (generate_ptr == NULL) {
		printf("error loading symbol from fractal.so\n");
	}
	int width  = atoi(argv[1]),
	    height = atoi(argv[2]);
	unsigned char *buf = (unsigned char*)malloc(width*height);
	generate(buf, width, height, 0, 0, width, height, 50.0, 50, -1.80, -0.09, -1.70, 0.01);

	generate_ptr(buf, width, height, 50, 50, 10, 10, 50.0, 50, -1, 1, -1, 1);
	//generate(buf, width, height, width, -1.5, 1.5, -1.5, 1.5);
	print_xpm(buf, width, height, stdout);
	fprintf(stderr, "%lf\n", maxn);
	dlclose(lib);
	return 0;
}
