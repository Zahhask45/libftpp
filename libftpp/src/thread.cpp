#include "thread.hpp"
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
	if (!_thread.joinable())
		return ;
	_thread.join();
}


void Thread::_trueStart(const std::string& name, const std::function<void()> &func){
	threadSafeCout.setPrefix(name);
	_func();
}
