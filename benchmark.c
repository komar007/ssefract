#include <stdbool.h>
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dlfcn.h>
#include <time.h>

typedef void (*generate_fun_t)(
		int *buf,
		int width, int height,
		int frame_x, int frame_y, int frame_w, int frame_h,
		double min_x, double min_y, double max_x, double max_y,
		double N, int iter,
		int C, int colors[], int def_color
);

generate_fun_t generate_asm = NULL,
	       generate_c   = NULL;


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
	void *lib_asm = dlopen("./fractal_asm.so", RTLD_LAZY);
	void *lib_c   = dlopen("./fractal_c.so",   RTLD_LAZY);
	generate_asm = dlsym(lib_asm, "generate");
	generate_c   = dlsym(lib_c,   "generate");
	if (!generate_asm) {
		printf("error loading symbol from fractal_asm.so\n");
		return -1;
	}
	if (!generate_c) {
		printf("error loading symbol from fractal_c.so\n");
		return -1;
	}
	generate_fun_t functions[] = {generate_asm, generate_c};
	int colors[768];
	for (int i = 0; i < 768; ++i)
		colors[i] = (i/3) | (((i+1)/3) << 8) | (((i+2)/3) << 16);
	for (int size = 0; size <= 2048*2048; size *= 2) {
		int width  = sqrt(size),
		    height = sqrt(size);
		int *buf = (int*)malloc(width*height*sizeof(int));
		printf("%i", size);
		for (int i = 0; i < sizeof(functions)/sizeof(*functions); ++i) {
			clock_t time_beg = clock();
			functions[i](
					buf,
					width, height,
					0, 0, width, height,
					-1.80, -0.09, -1.70, 0.01,
					50.0, 50,
					768, colors, 0xffffff
			);
			int time = clock() - time_beg;
			printf(" %i", time);
		}
		printf("\n");
		free(buf);
	}
	dlclose(lib_c);
	dlclose(lib_asm);
	return 0;
}
