#include "random_2D_coordinate_generator.hpp"

Random2DCoordinateGenerator::Random2DCoordinateGenerator(){
	this->_seed = std::random_device{}();
}

Random2DCoordinateGenerator::Random2DCoordinateGenerator(const long long& newSeed): _seed(newSeed){}

long long Random2DCoordinateGenerator::seed(){
	return this->_seed;
}

long long Random2DCoordinateGenerator::operator()(const long long& x, const long long& y){
	std::seed_seq seq{this->_seed, x, y};
	std::mt19937_64 prng(seq);

	return prng();
}
