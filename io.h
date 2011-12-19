#pragma once

#include <stdio.h>

void print_xpm(const int *buf, int width, int height, FILE *fp);
void print_bmp(const int *buf, int width, int height, FILE *fp);
