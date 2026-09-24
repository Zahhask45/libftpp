#include "persistent_worker.hpp"
#include <stdexcept>
#include <utility>

PersistentWorker::~PersistentWorker(){
	std::vector<std::thread> tasks;
	{
	
		std::lock_guard<std::mutex> lock(_guard);
		
		for (auto &fin : _toRemove){
			fin.second = true;
		}

		for (auto &fin : _tasks){
			tasks.push_back(std::move(fin.second));
		}

		_tasks.clear();
	}
	
	for (auto &task : tasks){
		if (task.joinable())
			task.join();
	}
}


void PersistentWorker::addTask(const std::string &name, const std::function<void()> &jobToExecute){
	std::lock_guard<std::mutex> lock(_guard);

	if (_tasks.find(name) != _tasks.end())
		throw std::runtime_error("Persistent Worker: task already exists.");

	_toRemove[name] = false;
	_tasks.emplace(name, std::thread([this, name, jobToExecute]() {
		while (true){
			{
				std::lock_guard<std::mutex> guard(_guard);
				if (_toRemove[name])
					return ;
			}
			jobToExecute();
		}
	}));
}

void PersistentWorker::removeTask(const std::string &name){
	std::thread task;

	{
		std::lock_guard<std::mutex> lock(_guard);

		auto taskIt = _tasks.find(name);
		if (taskIt == _tasks.end())
			throw std::runtime_error("Persistent Worker: unknown task.");

		_toRemove[name] = true;
		task = std::move(taskIt->second);
		_tasks.erase(taskIt);
	}

	if (task.joinable())
		task.join();

	{
		std::lock_guard<std::mutex> lock(_guard);
		_toRemove.erase(name);
	}
}
