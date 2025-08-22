#ifndef IVECTOR2_HPP
#define IVECTOR2_HPP

#include <cmath>

template<typename TType>
struct IVector2{
	TType x;
	TType y;

	IVector2(TType x, TType y);

	float length();
	IVector2<float> normalize();
	float dot(IVector2& v2);
	IVector2<TType> cross();

	IVector2<TType> operator+(IVector2<TType>& v2);
	IVector2<TType> operator-(IVector2<TType>& v2);
	IVector2<TType> operator*(IVector2<TType>& v2);
	IVector2<TType> operator/(IVector2<TType>& v2);
	bool operator==(IVector2<TType>& v2);
	bool operator!=(IVector2<TType>& v2);
};



#endif
