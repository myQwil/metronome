#include <cmath>

static const int run = 2048;
typedef float real;

class Slide
{
private:
	real m;
	real b;
	bool islog;

public:
	real min;
	real max;
	real val;

	Slide(real min, real max, real val, bool islog=true)
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
			m = log(max / min) / run;
		} else {
			m =    (max - min) / run;
		}
		this->b = min;

		if (min > max) {
			real temp = min;
			min = max;
			max = temp;
		}
		this->min = min;
		this->max = max;
	}

	real fromstep(int x)
	{
		if (islog) {
			return exp(m * x) * b;
		} else {
			return     m * x  + b;
		}
	}

	int tostep()
	{
		if (islog) {
			return (int)(log(val / b) / m);
		} else {
			return (int)(   (val - b) / m);
		}
	}
};
