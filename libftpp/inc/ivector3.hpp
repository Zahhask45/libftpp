#ifndef IVECTOR3_HPP
#define IVECTOR3_HPP

#include <cmath>

template<typename TType>
struct IVector3{
	TType x;
	TType y;
	TType z;

	IVector3(TType x, TType y, TType z);

	float length();
	IVector3<float> normalize();
	float dot(IVector3& v2);
	IVector3<TType> cross(IVector3<TType>& v2);

	IVector3<TType> operator+(IVector3<TType>& v2);
	IVector3<TType> operator-(IVector3<TType>& v2);
	IVector3<TType> operator*(IVector3<TType>& v2);
	IVector3<TType> operator/(IVector3<TType>& v2);
	bool operator==(IVector3<TType>& v2);
	bool operator!=(IVector3<TType>& v2);
};



#endif
