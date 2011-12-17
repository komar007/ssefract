#include <stdbool.h>
#include <complex.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <dlfcn.h>

double maxn;

int (*compute_point)(double complex *c, double *N, int k, double *abs) = NULL;

void generate(unsigned char *buf, int width, int height,
		double min_x, double max_x,
		double min_y, double max_y)
{
	maxn = 0;
	double range_x = max_x - min_x,
	       range_y = max_y - min_y;
	double step_x = range_x / width,
	       step_y = range_y / height;
	double N = 50;
	for (int j = 0; j < height; ++j) {
		for (int i = 0; i < width; ++i) {
			double complex c, z;
			c = (min_x + step_x * i) + (min_y + step_y*j)*I;
			buf[width*j+i] = 255;
#ifdef C
			z = 0 + 0*I;
			for (int k = 0; k < 50; ++k) {
				double abs;
				if ((abs = cabs(z)) > N) {
					double v = k - log2(log2(abs)/log2(N));
					if (v > maxn)
						maxn = v;
					buf[width*j+i] = 5*v;
					break;
				}
				z = fabs(creal(z)) + I*fabs(cimag(z));
				z = z*z + c;
			}
#else
			double v;
			int k = compute_point(&c, &N, 50, &v);
			if (k != 0) {
				if (v > maxn)
					maxn = v;
				buf[width*j+i] = 5*v;
			}
#endif
		}
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
	void *lib = dlopen("./test.so", RTLD_LAZY);
	compute_point = dlsym(lib, "compute_point");
	if (compute_point == NULL) {
		printf("error loading symbol from test.so\n");
	}
	int width  = atoi(argv[1]),
	    height = atoi(argv[2]);
	unsigned char *buf = (unsigned char*)malloc(width*height);
	generate(buf, width, height, -1.80, -1.70, -0.09, 0.01);
	//generate(buf, width, height, -1.5, 1.5, -1.5, 1.5);
	print_xpm(buf, width, height, stdout);
	fprintf(stderr, "%lf\n", maxn);
	dlclose(lib);
	return 0;
}
