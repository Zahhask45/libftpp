#include "data_buffer.hpp"
#include "pool.hpp"

template <typename T>
uintptr_t operator<<(DataBuffer& data, T to_seri){
	data.raw.push(reinterpret_cast<uintptr_t>(to_seri));
	return reinterpret_cast<uintptr_t>(to_seri);
}
template <typename T>
T operator>>(DataBuffer& data, T to_deseri){
	to_deseri = reinterpret_cast<T>(data.raw);
	return reinterpret_cast<T>(data.raw);	
}


