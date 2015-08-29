#include "io.h"
#include <stdint.h>
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
	int *tmp = malloc(sizeof(int)*num_colors);
	for (int i = 0; i < num_colors; ++i)
		fread(tmp+i, 3, 1, fp);
	fclose(fp);

	int new_num_colors = (num_colors-1)*8 + 1;
	*palette = malloc(new_num_colors*sizeof(int));
	for (int i = 0; i < new_num_colors; ++i) {
		int a = tmp[i/8];
		int b = tmp[i/8+1];
		double alpha = (i%8)/8.0;
		for (int j = 0; j < 4; ++j)
			((uint8_t*)&((*palette)[i]))[j] = ((uint8_t*)&a)[j] +
				alpha * (((uint8_t*)&b)[j] - ((uint8_t*)&a)[j]);
	}
	free(tmp);
	return new_num_colors;
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

struct mark_v0 {
	bool used;
	struct viewport viewport;
};
int load_marks(const char *filename, struct mark *marks)
{
	FILE *fp = fopen(filename, "r");
	if (!fp)
		return -1;
	uint8_t hdr[4];
	int n = fread(hdr, 4, 1, fp);
	if (n != 1)
		return -1;
	if (hdr[0] != 'M' || hdr[1] != 'A'|| hdr[2] != 'R') {
		/* compatibility mode */
		uint32_t nmarks = *(uint32_t*)hdr;
		struct mark_v0 *marks_old = calloc(nmarks, sizeof(struct mark_v0));
		n = fread(marks_old, sizeof(struct mark_v0), nmarks, fp);
		if (n != nmarks)
			return -1;
		fclose(fp);
		for (int i = 0; i < nmarks; ++i) {
			marks[i].used     = marks_old[i].used;
			marks[i].viewport = marks_old[i].viewport;
		}
		return nmarks;
	}
	int ver = hdr[3];
	if (ver != 1)
		return -1;
	uint32_t nmarks;
	n = fread(&nmarks, sizeof(uint32_t), 1, fp);
	if (n != 1)
		return -1;
	n = fread(marks, sizeof(struct mark), nmarks, fp);
	if (n != nmarks)
		return -1;
	fclose(fp);
	return nmarks;
}
