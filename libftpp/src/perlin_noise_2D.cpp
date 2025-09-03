#include "perlin_noise_2D.hpp"

PerlinNoise2D::PerlinNoise2D(){
	this->permutation.resize(256);
	std::srand(time(NULL));
	
	for (int i = 0; i < 256; i++){
		this->permutation[i] = i;
	}

	shuffleTable();
	
	this->permutation.resize(512);
	for (int i = 0; i < 256; i++){
		this->permutation[256 + i] = permutation[i];
	}
	
}

void PerlinNoise2D::shuffleTable(){
	for (int pos = this->permutation.size() - 1; pos >= 0; pos--){
		const int index = std::rand() % (pos + 1);
		std::swap(permutation[pos], permutation[index]);
	}
}


float PerlinNoise2D::fade(float t) const{
	return (t * t * t *(t *(t * 6.0f - 15.0f) + 10.0f));
}

float PerlinNoise2D::sample(float x, float y){
	IVector2<float> v(x, y);
	IVector2<float> v0(std::floor(x), std::floor(y));
	IVector2<float> v1(v0.x + 1.0f, v0.y + 0.0f);
	IVector2<float> v2(v0.x + 0.0f, v0.y + 1.0f);
	IVector2<float> v3(v0.x + 1.0f, v0.y + 1.0f);


	const int xindex = static_cast<int>(v0.x) & 255;
	const int yindex = static_cast<int>(v0.y) & 255;
	const int hash0 = permutation[(permutation[xindex	] + yindex) & 255];
	const int hash1 = permutation[(permutation[xindex + 1] + yindex) & 255];
	const int hash2 = permutation[(permutation[xindex	] + yindex + 1) & 255];
	const int hash3 = permutation[(permutation[xindex + 1] + yindex + 1) & 255];
	
	IVector2<float> g0 = grad(hash0);
	IVector2<float> g1 = grad(hash1);
	IVector2<float> g2 = grad(hash2);
	IVector2<float> g3 = grad(hash3);
	
	float tx = v.x - v0.x;
	float fx = fade(tx);
	float ty = v.y - v0.y;
	float fy = fade(ty);

	IVector2<float> m0 = v - v0;
	IVector2<float> m1 = v - v1;
	IVector2<float> m2 = v - v2;
	IVector2<float> m3 = v - v3;
	float v0v1 = (1.0f - fx) * g0.dot(m0) + fx * g1.dot(m1);
	float v2v3 = (1.0f - fx) * g2.dot(m2) + fx * g3.dot(m3);

	return ((1.0f - fy) * v0v1 + fy * v2v3);
}

float PerlinNoise2D::operator()(float x, float y){
	return (sample(x, y));
}

IVector2<float> PerlinNoise2D::grad(int hash){
	int h = hash & 3;
	switch (h){
		case 0: return IVector2<float>( 1.0f,  1.0f);
		case 1: return IVector2<float>(-1.0f,  1.0f);
		case 2: return IVector2<float>( 1.0f, -1.0f);
		default: return IVector2<float>(-1.0f, -1.0f);
		// case 4: return IVector2<float>( 1.0f,  0.0f);
		// case 5: return IVector2<float>(-1.0f,  0.0f);
		// case 6: return IVector2<float>( 0.0f,  1.0f);
		// default:return IVector2<float>( 0.0f, -1.0f);
	}
}
