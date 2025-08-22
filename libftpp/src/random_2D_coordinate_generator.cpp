#include "random_2D_coordinate_generator.hpp"

Random2DCoordinateGenerator::Random2DCoordinateGenerator(){
	this->value = time(NULL);
}

long long Random2DCoordinateGenerator::seed(){
	return this->value;
}
