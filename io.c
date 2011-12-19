#include "io.h"

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

