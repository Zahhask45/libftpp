#include "message.hpp"

Message::Message(int type)
    : _type(type),
      _sstream(std::ios_base::in | std::ios_base::out | std::ios_base::binary) {
}

Message::Type Message::getType(){
  return _type;
}
