#include <complex.h>
#include <math.h>

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
