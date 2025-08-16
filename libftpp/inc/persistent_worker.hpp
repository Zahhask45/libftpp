#ifndef PERSISTENT_WORKER_HPP
#define PERSISTENT_WORKER_HPP

#include <string>
#include <functional>
#include <unordered_map>
#include "thread_safe_queue.hpp"

class PersistentWorker {
	public:
		PersistentWorker() = default;
		~PersistentWorker();
		void addTask(const std::string &name, const std::function<void()> &jobToExecute);
		void removeTask(const std::string &name);
		
	private:
		std::mutex _guard;
		std::unordered_map<std::string, std::thread> _tasks;
		std::unordered_map<std::string, bool> _toRemove;
};
#endif
