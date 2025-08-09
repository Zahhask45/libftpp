#ifndef WORKER_POOL_HPP
#define WORKER_POOL_HPP

#include <functional>
#include <thread>
#include <mutex>
#include <string>

class IJobs{
	public:
		virtual void addJob(const std::function<void()>&);
		~IJobs();
};

#endif
