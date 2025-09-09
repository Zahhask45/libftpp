#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <memory>
#include <stdexcept>

template <typename TType>
class Singleton{
	public:
		static TType* instance(){
			if (!_obj) throw NoInstanceException();
			return _obj.get();
		};
		
		template <typename ... TArgs>
		static void instantiate(TArgs&&... p_args){
			if (_obj) throw AlreadyInstanceException();
			_obj.reset(new TType(std::forward<TArgs>(p_args)...));
		};
		
	private:
		static std::unique_ptr<TType> _obj;
		
	protected:
		Singleton() = default;
		virtual ~Singleton() = default;

	public:
		class NoInstanceException: public std::exception {
			public:
				const char *what() const noexcept {return "Singleton: Instance not created.";}
		};

		class AlreadyInstanceException: public std::exception{
			public:
				const char *what() const noexcept {return "Singleton: Instance already created";}
		};
};

template <typename TType>
std::unique_ptr<TType> Singleton<TType>::_obj = nullptr;

#endif
