#ifndef IVECTOR3_HPP
#define IVECTOR3_HPP

#include <cmath>

template<typename TType>
struct IVector3{
	TType x;
	TType y;
	TType z;

	IVector3(TType x = 0, TType y = 0, TType z = 0);

	float length() const;
	IVector3<float> normalize() const;
	float dot(const IVector3& v2) const;
	IVector3<TType> cross(const IVector3<TType>& v2) const;

	IVector3<TType> operator+(const IVector3<TType>& v2) const;
	IVector3<TType> operator-(const IVector3<TType>& v2) const;
	IVector3<TType> operator*(const IVector3<TType>& v2) const;
	IVector3<TType> operator/(const IVector3<TType>& v2) const;
	bool operator==(const IVector3<TType>& v2) const;
	bool operator!=(const IVector3<TType>& v2) const;
};



#endif
