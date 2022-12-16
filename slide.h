#include <cmath>

static const int steps = 2048;

struct Slide
{
	double k;
	double min;
	double val;
	bool islog;
	Slide(double min, double max, double val, bool islog=true)
	{
		this->val = val;
		this->islog = islog;
		if (islog) {
			if (min == 0.0 && max == 0.0) {
				max = 1.0;
			}
			if (max > 0.0) {
				if (min <= 0.0) {
					min = 0.01 * max;
				}
			} else {
				if (min > 0.0) {
					max = 0.01 * min;
				}
			}
			k = log(max / min) / steps;
		} else {
			k =    (max - min) / steps;
		}
		this->min = min;
	}

	double fromstep(int step)
	{
		if (islog) {
			return exp(k * step) * min;
		} else {
			return     k * step  + min;
		}
	}

	int tostep()
	{
		if (islog) {
			return (int)(log(val / min) / k);
		} else {
			return (int)(   (val - min) / k);
		}
	}
};
