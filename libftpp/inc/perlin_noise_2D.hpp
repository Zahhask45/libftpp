#ifndef PERLIN_NOISE_2D
#define PERLIN_NOISE_2D

#include "ivector2.hpp"
#include <vector>

class PerlinNoise2D{
	public:
		PerlinNoise2D();
		~PerlinNoise2D();
		float sample(float x, float y);
		float operator()(float x, float y);
	private:
		std::vector<int> permutation; 
};

#endif
