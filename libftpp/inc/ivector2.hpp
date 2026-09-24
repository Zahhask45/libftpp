#ifndef IVECTOR2_HPP
#define IVECTOR2_HPP

#include <cmath>

template<typename TType>
struct IVector2{
	TType x;
	TType y;


	IVector2(TType x = 0, TType y = 0);

	float length() const;
	IVector2<float> normalize() const;
	float dot(const IVector2& v2) const;
	IVector2<TType> cross() const;

	IVector2<TType> operator+(const IVector2<TType>& v2) const;
	IVector2<TType> operator-(const IVector2<TType>& v2) const;
	IVector2<TType> operator*(const IVector2<TType>& v2) const;
	IVector2<TType> operator/(const IVector2<TType>& v2) const;
	bool operator==(const IVector2<TType>& v2) const;
	bool operator!=(const IVector2<TType>& v2) const;
};



#endif
