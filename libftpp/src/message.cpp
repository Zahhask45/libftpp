#include "message.hpp"

Message::Message(int type)
    : _type(type),
      _sstream(std::ios_base::in | std::ios_base::out | std::ios_base::binary) {
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
