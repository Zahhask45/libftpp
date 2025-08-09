#include "thread.hpp"


Thread::Thread(const std::string& name, std::function<void()> functToExecute): _name(name), _functToExecute(functToExecute){}

void Thread::start(){
	_thread = std::thread(&Thread::_trueStart, _name, _functToExecute);
}

void Thread::stop(){
	_thread.join();
}


void Thread::_trueStart(const std::string& name, const std::function<void()> &functToExecute){
	threadSafeCout.setPrefix(name);
	functToExecute();
}
