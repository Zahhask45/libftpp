#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "data_buffer.hpp"
#include <stdexcept>

class Message {
  public:
    using Type = int;
    Type type();

    explicit Message(Type type);

    template<typename TType>
    friend Message &operator<<(Message &msg, const TType &obj) {
      msg._data << obj;
      return msg;
    };
  
    template<typename TType>
    friend const Message &operator>>(Message &msg, TType & obj){
      msg._data >> obj;
      return msg;
    };

    std::string serialize() const;

    void deserialize(const std::string &str);
    
  private: 
    Type _type;
    DataBuffer _data;

  public:
    class DeserializationFailedException : public std::runtime_error {
      public:
        explicit DeserializationFailedException(const std::string &msg):
          runtime_error("Message: " + msg + "."){}
    };
};

#endif
