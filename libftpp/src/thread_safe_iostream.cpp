#include "thread_safe_iostream.hpp"

thread_local ThreadSafeIOStream threadSafeCout;
// std::mutex ThreadSafeIOStream::_mutex;

ThreadSafeIOStream &
ThreadSafeIOStream::operator<<(std::ostream &(*manipulator)(std::ostream &)) {
  const std::lock_guard<std::mutex> lock(_mutex);
  // std::ostringstream buffer;

  std::cout << _prefix << _buffer.str();

  manipulator(std::cout);
  _buffer.str("");
  _buffer.clear();
  return (*this);
}

void ThreadSafeIOStream::setPrefix(const std::string &prefix) {
  const std::lock_guard<std::mutex> lock(_mutex);
  _prefix = prefix;
}
