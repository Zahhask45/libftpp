#ifndef WORKER_POOL_HPP
#define WORKER_POOL_HPP

#include "thread_safe_queue.hpp"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class IJobs {
public:
  virtual void addJob(const std::function<void()> &jobToExecute) = 0;
  ~IJobs() {};
};

class WorkerPool : public IJobs {
public:
  WorkerPool(WorkerPool &) = delete;
  WorkerPool &operator=(const WorkerPool &) = delete;
  WorkerPool(int amount);
  ~WorkerPool();
  void addJob(const std::function<void()> &jobToExecute) override;
  int checker();

private:
  void _startThread();
  std::vector<std::thread> _workers;
  ThreadSafeQueue<std::function<void()>> _queue;
  std::mutex _queueMutex;
  std::mutex _guard;
  bool _stopthat;
  std::condition_variable _condition;
};

#endif
