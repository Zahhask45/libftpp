#include "worker_pool.hpp"
#include <iostream>

WorkerPool::WorkerPool(int amount) {
  _workers.reserve(amount);
  for (size_t i = 0; i < static_cast<size_t>(amount); i++) {
    _workers.emplace_back(&WorkerPool::_startThread, this);
  }
}
WorkerPool::~WorkerPool() {
  for (auto &worker : _workers) {
    if (worker.joinable())
      worker.join();
  }
}

void WorkerPool::addJob(const std::function<void()> &jobToExecute) {
  _queue.push_back(jobToExecute);
}

void WorkerPool::_startThread() {
  while (true) {
    std::function<void()> job;
    std::unique_lock<std::mutex> lock(_queueMutex);
    //
    //
    job = _queue.pop_front();
    try {
      job();
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }
}
