#include "thread.hpp"
#include "thread_safe_iostream.hpp"
#include <stdexcept>


Thread::Thread(const std::string& name, std::function<void()> func)
	: _name(name), _func(std::move(func)){}

void Thread::start(){
	if (_thread.joinable())
		throw std::runtime_error("Thread: already started");
	if (!_func)
		throw std::runtime_error("Thread: no function assigned");
		
	_thread = std::thread(&Thread::run, this);
}

void Thread::stop(){
	if (_thread.joinable())
		_thread.join();
}


void Thread::run(){
	threadSafeCout.setPrefix(_name);
	_func();
}
