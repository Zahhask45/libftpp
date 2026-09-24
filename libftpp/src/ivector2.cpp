#include "ivector2.hpp"

template<typename TType>
IVector2<TType>::IVector2(TType x, TType y){
	this->x = x;
	this->y = y;
}

template<typename TType>
float IVector2<TType>::length() const{
	return (sqrt(static_cast<float>(x*x + y*y)));
}

template<typename TType>
IVector2<float> IVector2<TType>::normalize(){
	float len = this->length();
	if (len == 0.0f)
		return IVector2<float>(0.0f, 0.0f);
	
	return IVector2<float>(static_cast<float>(x / len), static_cast<float>(y / len));
}

template<typename TType>
float IVector2<TType>::dot(IVector2<TType>& v2){
	return static_cast<float>(x * v2.x + y * v2.y);
}

template<typename TType>
IVector2<TType> IVector2<TType>::cross(){
	return IVector2<TType>(-y, x);
}

template<typename TType>
IVector2<TType> IVector2<TType>::operator+(IVector2<TType>& v2){
	return IVector2<TType>(this->x + v2.x, this->y + v2.y);
}

template<typename TType>
IVector2<TType> IVector2<TType>::operator-(IVector2<TType>& v2){
	return IVector2<TType>(this->x - v2.x, this->y - v2.y);
}

template<typename TType>
IVector2<TType> IVector2<TType>::operator*(IVector2<TType>& v2){
	return IVector2<TType>(this->x * v2.x, this->y * v2.y);
}

template<typename TType>
IVector2<TType> IVector2<TType>::operator/(IVector2<TType>& v2){
	return IVector2<TType>(this->x / v2.x, this->y / v2.y);
}

template<typename TType>
bool IVector2<TType>::operator==(IVector2<TType>& v2){
	return (this->x == v2.x && this->y == v2.y);
}

template<typename TType>
bool IVector2<TType>::operator!=(IVector2<TType>& v2){
	return !(*this == v2);
}

template struct IVector2<int>;
template struct IVector2<float>;
