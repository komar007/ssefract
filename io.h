#pragma once

#include "browser.h"
#include <stdio.h>

void print_xpm(const int *buf, int width, int height, FILE *fp);
void print_bmp(const int *buf, int width, int height, FILE *fp);
int load_palette(const char *filename, int**);
int dump_marks(const char *filename, int nmarks, const struct mark *marks);
int load_marks(const char *filename, struct mark *marks);
