#include "message.hpp"

Message::Message(Type type)
    : _type(type),
      _sstream(std::ios_base::in | std::ios_base::out | std::ios_base::binary) {
      std::cout << "Message created with type: " << _type << std::endl;
}

Message::Message(const Message& other): _type(other._type),
      _sstream(std::ios_base::in | std::ios_base::out | std::ios_base::binary) {
  _sstream << other._sstream.rdbuf();
}

Message &Message::operator=(const Message &message){
  this->_type = message._type;
  this->_sstream << message._sstream.rdbuf();
  return *this;
}

Message::Type Message::getType(){
  return _type;
}

std::string Message::serialize() const{
  std::ostringstream oss;

  oss << _type << " ";
  oss << _sstream.rdbuf();
  return oss.str();
}

Message Message::deserialize(std::stringstream &ss){
  Type type;
  ss >> type;

  Message msg(type);
  std::string str;
  msg._sstream << ss.rdbuf();
  return msg;
}
