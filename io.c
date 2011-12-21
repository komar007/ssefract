#include "io.h"
#include <stdlib.h>

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

int load_palette(const char *filename, int **palette)
{
	FILE *fp = fopen(filename, "r");
	fseek(fp, 18, SEEK_SET);
	int num_colors;
	fread(&num_colors, 4, 1, fp);
	fseek(fp, 54, SEEK_SET);
	*palette = malloc(sizeof(int)*num_colors);
	for (int i = 0; i < num_colors; ++i)
		fread(*palette+i, 3, 1, fp);
	fclose(fp);
	return num_colors;;
}

int dump_marks(const char *filename, int nmarks, const struct mark *marks)
{
	FILE *fp = fopen(filename, "w");
	if (!fp)
		return -1;
	int n = fwrite(&nmarks, sizeof(int), 1, fp);
	if (n != 1)
		return -1;
	n = fwrite(marks, sizeof(struct mark), nmarks, fp);
	if (n != nmarks)
		return -1;
	fclose(fp);
	return 0;
}
int load_marks(const char *filename, struct mark *marks)
{
	FILE *fp = fopen(filename, "r");
	if (!fp)
		return -1;
	int nmarks;
	int n = fread(&nmarks, sizeof(int), 1, fp);
	if (n != 1)
		return -1;
	n = fread(marks, sizeof(struct mark), nmarks, fp);
	if (n != nmarks)
		return -1;
	fclose(fp);
	return nmarks;
}
