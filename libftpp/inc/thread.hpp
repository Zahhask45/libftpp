#ifndef THREAD_HPP
#define THREAD_HPP

#include <thread>
#include <functional>
#include <string>

class Thread{
	public:
		Thread(const std::string& name, std::function<void()> func);
		void start();
		void stop();
	private:
		void run();
	private:
		std::string _name;
		std::function<void()> _func;
		std::thread _thread;
};

#endif
