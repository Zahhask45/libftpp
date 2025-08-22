#ifndef RANDOM_2D_COORDINATE_GENERATOR
#define RANDOM_2D_COORDINATE_GENERATOR

#include <iostream>

struct Random2DCoordinateGenerator{
	Random2DCoordinateGenerator();
	long long seed();
	long long operator()(const long long& x, const long long& y);
	long long value;
};

#endif
