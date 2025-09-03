#include "ivector3.hpp"

template<typename TType>
IVector3<TType>::IVector3(TType x, TType y, TType z){
	this->x = x;
	this->y = y;
	this->z = z;
}

template<typename TType>
float IVector3<TType>::length(){
	return (sqrt(x*x + y*y + z*z));
}

template<typename TType>
IVector3<float> IVector3<TType>::normalize(){
	float len = this->length();
	IVector3<float> norm(this->x / len, this->y / len, this->z / len);
	return norm;
}

template<typename TType>
float IVector3<TType>::dot(IVector3<TType>& v2){
	return (this->x * v2.x + this->y * v2.y + this->z * v2.z);
}

template<typename TType>
IVector3<TType> IVector3<TType>::cross(IVector3<TType>& v2){
	TType cross_x = this->y * v2.z - this->z * v2.y;
	TType cross_y = this->z * v2.x - this->x * v2.z;
	TType cross_z = this->x * v2.y - this->y * v2.x;

	
	IVector3<TType> crossed(cross_x, cross_y, cross_z);
	return crossed;
}

template<typename TType>
IVector3<TType> IVector3<TType>::operator+(IVector3<TType>& v2){
	IVector3<TType> addVec(this->x + v2.x, this->y + v2.y, this->z + v2.z);
	return addVec;
}

template<typename TType>
IVector3<TType> IVector3<TType>::operator-(IVector3<TType>& v2){
	IVector3<TType> subVec(this->x - v2.x, this->y - v2.y, this->z - v2.z);
	return subVec;
}

template<typename TType>
IVector3<TType> IVector3<TType>::operator*(IVector3<TType>& v2){
	IVector3<TType> mulVec(this->x * v2.x, this->y * v2.y, this->z * v2.z);
	return mulVec;
}

template<typename TType>
IVector3<TType> IVector3<TType>::operator/(IVector3<TType>& v2){
	IVector3<TType> divVec(this->x / v2.x, this->y / v2.y, this->z / v2.z);
	return divVec;
}

template<typename TType>
bool IVector3<TType>::operator==(IVector3<TType>& v2){
	return (this->x == v2.x && this->y == v2.y && this->z == v2.z);
}

template<typename TType>
bool IVector3<TType>::operator!=(IVector3<TType>& v2){
	return (this->x != v2.x && this->y != v2.y && this->z != v2.z);
}

template struct IVector3<int>;
template struct IVector3<float>;
