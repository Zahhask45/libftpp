#ifndef PERLIN_NOISE_2D
#define PERLIN_NOISE_2D

#include "ivector2.hpp"
#include <vector>
#include <iostream>
#include <cstdlib>
#include <time.h>

class PerlinNoise2D{
	public:
		PerlinNoise2D();
		float sample(float x, float y);
		float operator()(float x, float y);
	private:
		void shuffleTable();
		float fade(float t) const;
		IVector2<float> grad(int hash);
	private:
		std::vector<int> permutation;
};

#endif
