#ifndef THREAD_HPP
#define THREAD_HPP

#include <thread>
#include <functional>
#include <string>
#include "thread_safe_iostream.hpp"

class Thread{
	public:
		Thread(const std::string& name, std::function<void()> functToExecute);
		void start();
		void stop();
	private:
		static void  _trueStart(const std::string& name, const std::function<void()> &functToExecute);
	private:
		std::string _name;
		std::function<void()> _functToExecute;
		std::thread _thread;
};

#endif
