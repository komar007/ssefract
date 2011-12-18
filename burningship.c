#include <stdbool.h>
#include <complex.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <dlfcn.h>


int (*compute_point)(double complex c, double N, int k, double *abs) = NULL;
void (*generate_ptr)(int *buf, int width, int height, int frame_x, int frame_y, int frame_w, int frame_h, double min_x, double min_y, double max_x, double max_y, double N, int iter, int C, int colors[], int def_color) = NULL;

void generate(
		int *buf,
		int width, int height,
		int frame_x, int frame_y,
		int frame_w, int frame_h,
		double min_x, double min_y,
		double max_x, double max_y,
		double N, int iter,
		int C, int colors[], int def_color
)
{
	double range_x = max_x - min_x,
	       range_y = max_y - min_y;
	double step_x = range_x / width,
	       step_y = range_y / height;
	double coef = (double)C / iter;
	buf += frame_y*width + frame_x;
	double N2 = N*N;
	for (int j = 0; j < frame_h; ++j) {
		double complex c = (min_x + step_x * frame_x) + (min_y + step_y * frame_y)*I;
		for (int i = 0; i < frame_w; ++i) {
			double complex z;
			buf[i] = def_color;
			z = 0 + 0*I;
			int k;
			double abs2;
			for (k = 0; k < iter; ++k) {
				z = fabs(creal(z)) + I*fabs(cimag(z));
				abs2 = creal(z)*creal(z)+cimag(z)*cimag(z);
				if (abs2 > N2) {
					break;
				}
				z = z*z + c;
			}
			if (k != iter) {
				double v = k - log2(log2(abs2)/log2(N2));
				buf[i] = colors[(int)round(v*coef)];
			}
			c += step_x + 0*I;

		}
		++frame_y;
		buf += width;
	}
}

void print_xpm(const int *buf, int width, int height, FILE *fp)
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

void print_bmp(const int *buf, int width, int height, FILE *fp)
{
	static const unsigned char BM_HDR1[] = {
		0x42, 0x4d, 0x36, 0xb8, 0x0b, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
		0x00, 0x00};
	static const unsigned char BM_HDR2[] = {
		0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0xb8, 0x0b, 0x00, 0x13, 0x0b, 0x00, 0x00,
		0x13, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	fwrite(BM_HDR1, 1, sizeof(BM_HDR1), fp);
	fwrite(&width,  4, 1, fp);
	fwrite(&height, 4, 1, fp);
	fwrite(BM_HDR2, 1, sizeof(BM_HDR2), fp);
	for (int j = height - 1; j >= 0; --j)
		for (int i = 0; i < width; ++i)
			fwrite(buf + j*width+i, 3, 1, fp);
}


int main(int argc, char *argv[])
{
	void *lib = dlopen("./fractal.so", RTLD_LAZY);
	compute_point = dlsym(lib, "compute_point");
	if (compute_point == NULL) {
		printf("error loading symbol from fractal.so\n");
	}
#ifdef LANG_C
	generate_ptr = &generate;
#else
	generate_ptr = dlsym(lib, "generate");
#endif
	if (generate_ptr == NULL) {
		printf("error loading symbol from fractal.so\n");
	}
	int width  = atoi(argv[1]),
	    height = atoi(argv[2]);
	int *buf = (int*)malloc(width*height*sizeof(int));
	int colors[256];
	for (int i = 0; i < 256; ++i)
		colors[i] = i | (i << 8) | (i << 16);
	generate_ptr(buf, width, height, 0, 0, width, height, -1.80, -0.09, -1.70, 0.01, 50.0, 50, 256, colors, 0xffffff);
	FILE *fp = fopen(argv[3], "w");
	print_bmp(buf, width, height, fp);
	fclose(fp);
	dlclose(lib);
	return 0;
}
