#ifndef POOL_HPP
#define POOL_HPP

#include <iostream>


template<typename TType>
class Pool{
	public:
		// nested class
		class Object{
			private:
				friend class Pool<TType>;
				Pool* pool;
				TType* ptr;
				size_t index;

				Object(Pool *p, TType* obj, size_t i): pool(p), ptr(obj), index(i){}
			public:
				~Object(){
					ptr->~TType();
					pool->used[index] = false;
				}
				
				TType* operator->(){
					return ptr;
				};
					
		};


	private:
		TType *obj;
		bool* used;
		size_t cap;
	public:
	// resizing my obj capacity
	void resize(const size_t& numberOfObjectsStored){
		cap = numberOfObjectsStored;
		obj = static_cast<TType*>(::operator new(sizeof(TType) * cap));
		used = new bool[cap]();
	};

	
	template<typename... TArgs>
	Object acquire(TArgs&&... p_args){
		for (size_t i = 0; i < cap; i++){
			if (!used[i]){
				TType* obj = new (&obj[i]) TType(std::forward<TArgs>(p_args)...);
				used[i] = true;
				return Object(this, obj, i);
			}
		}
		return Object(nullptr, nullptr, 0);
	}


	

	Pool(): obj(nullptr), used(nullptr), cap(0){}
	~Pool(){
		for (size_t i = 0; i < cap; i++){
			obj[i].~TType();
		}
		::operator delete(obj);
		delete[] used;
	}
	
};



#endif
