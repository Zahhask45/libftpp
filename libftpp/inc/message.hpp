#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <iostream>
#include <sstream>
#include <string>

class Message {
  public:
    using Type = int;
    int type();

    Message(int type);
    template<typename T>
    Message &operator<< (const T &obj) {
      const_cast<Message &>(*this)._sstream << obj;
      return *this;
    };
  
    template<typename T>
      const Message &operator >> (T & obj) const {
        const_cast<Message&>(*this)._sstream >> obj;
        return *this;
    };

    Type getType();
  private: 
    Type _type;
    std::stringstream _sstream;
};

#endif
