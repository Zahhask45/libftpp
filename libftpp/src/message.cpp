#include "message.hpp"

Message::Message(Type type)
    : _type(type){}

Message::Type Message::type(){
  return _type;
}

std::string Message::serialize() const{
  std::string seri;
  seri.resize(sizeof(_type) + _data.data().size());

  std::memcpy(seri.data(), &_type, sizeof(_type));
  std::memcpy(seri.data() + sizeof(_type), _data.data().data(), _data.data().size());

  return seri;
}

void Message::deserialize(const std::string &str){
  if (str.size() < sizeof(_type))
    throw DeserializationFailedException("Failed to receive data");

  std::memcpy(&_type, str.data(), sizeof(_type));

  _data.data().clear();
  _data.data().resize(str.size() - sizeof(_type));

  std::memcpy(_data.data().data(), str.data() + sizeof(_type), str.size() - sizeof(_type));
}
