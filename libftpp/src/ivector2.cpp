#include "ivector2.hpp"


template<typename TType>
IVector2<TType>::IVector2(TType x, TType y){
	this->x = x;
	this->y = y;
}

template<typename TType>
float IVector2<TType>::length(){
	return (sqrt(x*x + y*y));
}

template<typename TType>
IVector2<float> IVector2<TType>::normalize(){
	float len = this->length();
	IVector2<float> norm(this->x / len, this->y / len);
	return norm;
}

template<typename TType>
float IVector2<TType>::dot(IVector2<TType>& v2){
	return (this->x * v2.x + this->y * v2.y);
}

template<typename TType>
IVector2<TType> IVector2<TType>::cross(){
	IVector2<TType> crossed(-this->y, this->x);
	return crossed;
}

template<typename TType>
IVector2<TType> IVector2<TType>::operator+(IVector2<TType>& v2){
	IVector2<TType> addVec(this->x + v2.x, this->y + v2.y);
	return addVec;
}

template<typename TType>
IVector2<TType> IVector2<TType>::operator-(IVector2<TType>& v2){
	IVector2<TType> subVec(this->x - v2.x, this->y - v2.y);
	return subVec;
}

template<typename TType>
IVector2<TType> IVector2<TType>::operator*(IVector2<TType>& v2){
	IVector2<TType> mulVec(this->x * v2.x, this->y * v2.y);
	return mulVec;
}

template<typename TType>
IVector2<TType> IVector2<TType>::operator/(IVector2<TType>& v2){
	IVector2<TType> divVec(this->x / v2.x, this->y / v2.y);
	return divVec;
}

template<typename TType>
bool IVector2<TType>::operator==(IVector2<TType>& v2){
	return (this->x == v2.x && this->y == v2.y);
}

template<typename TType>
bool IVector2<TType>::operator!=(IVector2<TType>& v2){
	return (this->x != v2.x && this->y != v2.y);
}

template struct IVector2<int>;
template struct IVector2<float>;
