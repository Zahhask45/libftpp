#ifndef POOL_HPP
#define POOL_HPP

#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>


template<typename TType>
class Pool{
	public:
		// nested class
		class Object{
			private:
				friend class Pool;
				
				Pool<TType>* pool;
				TType* obj;

				Object(Pool<TType> *p_pool, TType* p_obj): pool(p_pool), obj(p_obj){}
				
			public:
				~Object(){
					obj->~TType();

					if (pool){
						if (obj)	
							pool->_availObj.push_back(obj);
					}
				}
				
				TType* operator->(){
					return obj;
				};
					
		};


	private:
		std::vector<std::unique_ptr<TType>> _allocObj;
		std::vector<TType*> _availObj;
		size_t _total = 0;

	public:
		void resize(const size_t& numberOfObjectsStored){
			if (numberOfObjectsStored > _total){
				// grow size
				size_t objToAlloc = numberOfObjectsStored - _total;

				while (objToAlloc > 0){
					auto newObj = std::make_unique<TType>();

					_availObj.push_back(newObj.get());
					_allocObj.push_back(std::move(newObj));

					_total++;
					objToAlloc--;
				}
			}else if(numberOfObjectsStored < _total){
				// shrink size
				size_t objToRem = _total - numberOfObjectsStored;

				while (objToRem > 0 && !_allocObj.empty()){
					if (_availObj.empty())
						NoAvailableObjToRemoveException();

					TType *obj = _availObj.back();
					_availObj.pop_back();

					auto it = std::find_if(_allocObj.begin(), _allocObj.end(), [obj](const std::unique_ptr<TType> &ptr) { return ptr.get() == obj;});

					if (it != _allocObj.end()){
						_allocObj.erase(it);
						_total--;
						objToRem--;
					}
				}
				if (objToRem > 0)
					throw NoAllocatedObjectsToRemoveException();
			}
		};

		
		template<typename... TArgs>
		Object acquire(TArgs&&... p_args){
			if (_availObj.empty())
				throw NoAvailableObjectsException();

			TType *obj = _availObj.back();
			_availObj.pop_back();

			new (obj) TType(std::forward<TArgs>(p_args)...);

			Object objectWrapper(this, obj);
			return objectWrapper;
		}


	

	public:
		class NoAvailableObjectsException : public std::runtime_error{
			public:
				explicit NoAvailableObjectsException():
					runtime_error("Pool: No Available Objects."){}
		};

		class NoAvailableObjToRemoveException : public std::runtime_error{
			public:
				explicit NoAvailableObjToRemoveException():
					runtime_error("Pool: No Available Objects to Remove."){}
		};

		class NoAllocatedObjectsToRemoveException : public std::exception{
			public:
				const char *what() const noexcept {return "Pool: No Allocated Objects to Remove.";}
		};
	
};



#endif
