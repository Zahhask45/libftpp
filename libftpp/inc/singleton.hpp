#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <sstream>

template <typename TType>
class Singleton{
	public:
		static TType* instance(){
			if (_obj == nullptr) throw std::runtime_error("empty");
			return _obj;
		};
		template <typename ... TArgs>
		static void instantiate(TArgs&&... p_args){
			if (_obj) throw std::runtime_error("Already created");
			_obj = new TType(p_args...);
		};
	private:
		static TType *_obj;	
};

template <typename TType>
TType* Singleton<TType>::_obj = nullptr;

#endif
