#include "ivector3.hpp"

template<typename TType>
IVector3<TType>::IVector3(TType x, TType y, TType z){
	this->x = x;
	this->y = y;
	this->z = z;
}

template<typename TType>
float IVector3<TType>::length() const{
	float fx = static_cast<float>(x);
	float fy = static_cast<float>(y);
	float fz = static_cast<float>(z);
	return (std::sqrt(fx*fx + fy*fy + fz*fz));
}

template<typename TType>
IVector3<float> IVector3<TType>::normalize() const{
	float len = this->length();
	if (len == 0.0f)
		return IVector3<float>(0.0f, 0.0f, 0.0f);
	return IVector3<float>(static_cast<float>(x) / len, static_cast<float>(y) / len, static_cast<float>(z) / len);
}

template<typename TType>
float IVector3<TType>::dot(const IVector3<TType>& v2) const{
	return (this->x * v2.x + this->y * v2.y + this->z * v2.z);
}

template<typename TType>
IVector3<TType> IVector3<TType>::cross(const IVector3<TType>& v2) const{
	TType cross_x = this->y * v2.z - this->z * v2.y;
	TType cross_y = this->z * v2.x - this->x * v2.z;
	TType cross_z = this->x * v2.y - this->y * v2.x;

	
	IVector3<TType> crossed(cross_x, cross_y, cross_z);
	return crossed;
}

template<typename TType>
IVector3<TType> IVector3<TType>::operator+(const IVector3<TType>& v2) const{
	IVector3<TType> addVec(this->x + v2.x, this->y + v2.y, this->z + v2.z);
	return addVec;
}

template<typename TType>
IVector3<TType> IVector3<TType>::operator-(const IVector3<TType>& v2) const{
	IVector3<TType> subVec(this->x - v2.x, this->y - v2.y, this->z - v2.z);
	return subVec;
}

template<typename TType>
IVector3<TType> IVector3<TType>::operator*(const IVector3<TType>& v2) const{
	IVector3<TType> mulVec(this->x * v2.x, this->y * v2.y, this->z * v2.z);
	return mulVec;
}

template<typename TType>
IVector3<TType> IVector3<TType>::operator/(const IVector3<TType>& v2) const{
	IVector3<TType> divVec(this->x / v2.x, this->y / v2.y, this->z / v2.z);
	return divVec;
}

template<typename TType>
bool IVector3<TType>::operator==(const IVector3<TType>& v2) const{
	return (this->x == v2.x && this->y == v2.y && this->z == v2.z);
}

template<typename TType>
bool IVector3<TType>::operator!=(const IVector3<TType>& v2) const{
	return !(*this == v2);
}

template struct IVector3<int>;
template struct IVector3<float>;
