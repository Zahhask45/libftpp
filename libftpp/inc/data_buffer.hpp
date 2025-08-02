#ifndef DATA_BUFFER_HPP
#define DATA_BUFFER_HPP

#include <memory>
#include <cstring>
#include <sstream>
#include "pool.hpp"

class DataBuffer{
	public:
		template <typename T>
		DataBuffer& operator<<(const T& to_seri){
			_raw << to_seri << " ";
			return *this;
		}
		template <typename T>
		DataBuffer& operator>>(T& to_deseri){
			if (_raw.eof() == true) throw std::runtime_error("empty");
			_raw >> to_deseri;	
			return *this;
		}
	
		
		DataBuffer();
		~DataBuffer();
	private:
		std::stringstream _raw;
};

#endif

