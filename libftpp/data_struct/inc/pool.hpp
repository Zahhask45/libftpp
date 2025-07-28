#ifndef POOL_HPP
#define POOL_HPP

#include <iostream>

template<typename TType>
class Pool{
	private:
		TType *obj;
	public:
		class Object{
			public:
				TType* operator->();
					
		};
	void resize(const size_t& numberOfObjectsStored){
		obj[5];
	};
	template<typename... TArgs>
	Object acquire(TArgs&&... p_args);
};



#endif
