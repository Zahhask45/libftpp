#include "worker_pool.hpp"
#include <iostream>

WorkerPool::WorkerPool(int amount) : _stopthat(false) {
  _workers.reserve(amount);
  for (size_t i = 0; i < static_cast<size_t>(amount); ++i) {
    _workers.emplace_back(&WorkerPool::_startThread, this);
  }
}
WorkerPool::~WorkerPool() {
  {
    std::lock_guard<std::mutex> lock(_queueMutex);
    _stopthat = true;
  }
  {
    std::unique_lock<std::mutex> lock(_queueMutex);
    _condition.notify_all();
  }
  for (auto &worker : _workers) {
    if (worker.joinable())
      worker.join();
  }
}

void WorkerPool::addJob(const std::function<void()> &jobToExecute) {
  {

    std::unique_lock<std::mutex> lock(_queueMutex);
    _queue.push_back(jobToExecute);
  }
  {
    std::lock_guard<std::mutex> lock(_queueMutex);
    _condition.notify_one();
  }
}

void WorkerPool::_startThread() {
  while (true) {
    std::function<void()> job;
    std::unique_lock<std::mutex> lock(_queueMutex);
    _condition.wait(lock,
                    [&]() { return (_stopthat || _queue.empty() == false); });
    {
      std::lock_guard<std::mutex> lock(_guard);
      if (_stopthat == true) {
        return;
      }
    }
    job = _queue.pop_front();
    try {
      job();
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }
}

int WorkerPool::checker() {
  {

    std::lock_guard<std::mutex> lock(_guard);
    if (_stopthat)
      return 1;
  }
  {
    std::unique_lock<std::mutex> lock(_queueMutex);
    if (_queue.empty() == false)
      return 1;
  }
  return 0;
}
