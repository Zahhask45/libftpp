#ifndef DATA_BUFFER_HPP
#define DATA_BUFFER_HPP

#include <queue>
#include <memory>
#include <cstring>
#include "pool.hpp"

class DataBuffer{
	public:
		template <typename T>
		friend uintptr_t operator<<(DataBuffer& data, T to_seri);
		template <typename T>
		friend T operator>>(DataBuffer& data, T to_deseri);
		
	private:
		std::queue<uintptr_t> raw;
};

template <typename T>
uintptr_t operator<<(DataBuffer& data, T to_seri);
template <typename T>
T operator>>(DataBuffer& data, T to_deseri);

#endif
