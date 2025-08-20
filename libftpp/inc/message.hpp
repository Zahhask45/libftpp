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
    Message(const Message& other);
    Message(){};
    template<typename T>
    Message &operator<< (const T &obj) {
      const_cast<Message &>(*this)._sstream << obj;
      return *this;
    };
  
    template<typename T>
      const Message &operator>> (T & obj) const {
        const_cast<Message&>(*this)._sstream >> obj;
        return *this;
    };

    Type getType();

    std::string serialize() const;
    static Message deserialize(std::stringstream &ss);
  private: 
    Type _type;
    std::stringstream _sstream;
};

#endif
