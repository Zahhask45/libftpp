#include "persistent_worker.hpp"

PersistentWorker::~PersistentWorker(){
	{
		std::lock_guard<std::mutex> lock(_guard);
		for (auto &fin : _toRemove){
			fin.second = true;
		}
	}
	for (auto &task : _tasks){
		if (task.second.joinable())
			task.second.join();
	}
}


void PersistentWorker::addTask(const std::string &name, const std::function<void()> &jobToExecute){
	_tasks.insert({name, std::thread([&](){
		while (true){
			bool is_finished;
			{
				std::lock_guard<std::mutex> lock(_guard);
				is_finished = _toRemove[name];
			}
			if (is_finished)
				return;
			jobToExecute();
		}
	})});
}

void PersistentWorker::removeTask(const std::string &name){
	{
		std::lock_guard<std::mutex> lock(_guard);
		_toRemove[name] = true;
	}
	auto &task = _tasks[name];

	if (task.joinable())
		task.join();
	_tasks.erase(name);
}
