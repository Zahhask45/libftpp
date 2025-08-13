#ifndef THREAD_SAFE_IOSTREAM_HPP
#define THREAD_SAFE_IOSTREAM_HPP

#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

class ThreadSafeIOStream {
public:
  void setPrefix(const std::string &prefix);

  template <typename T> ThreadSafeIOStream &operator<<(const T &p_prefix) {
    _buffer << p_prefix;
    return (*this);
  };

  // This will handle the manipulators std::endl, std::flush, etc.
  ThreadSafeIOStream &operator<<(std::ostream &(*manipulator)(std::ostream &));

  template <typename T> ThreadSafeIOStream &operator>>(T &obj) {
    const std::lock_guard<std::mutex> lock(_mutex);
    std::cout << _thread_prefix;

    std::cin >> obj;
    return (*this);
  };

  template <typename T> void prompt(const std::string &question, T &dest) {
    const std::lock_guard<std::mutex> lock(_mutex);
    std::cout << _thread_prefix;

    std::cout << question;
    std::cin >> dest;
  };

private:
  static std::mutex _mutex;
  std::string _thread_prefix;
  std::ostringstream _buffer;
};

extern thread_local ThreadSafeIOStream threadSafeCout;

#endif
