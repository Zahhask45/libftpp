#ifndef RANDOM_2D_COORDINATE_GENERATOR
#define RANDOM_2D_COORDINATE_GENERATOR

#include <random>

class Random2DCoordinateGenerator{
	public:
		Random2DCoordinateGenerator();
		Random2DCoordinateGenerator(const long long& newSeed);
		long long seed();
		long long operator()(const long long& x, const long long& y);
	private:
		long long _seed;
};

#endif
