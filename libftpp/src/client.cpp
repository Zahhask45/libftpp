#include "client.hpp"

void Client::connect(const std::string& address, const size_t& port){
	std::cout << "Connection being made to " << address << ":" << port << std::endl;
	std::string addressTemp = address;
	if ((_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		std::cerr << "Failed connection (socket)" << std::endl;
		return ;
	}

	_serv_addr.sin_family = AF_INET;
	_serv_addr.sin_port = htons(port);

	if (addressTemp == "localhost")
		addressTemp = "127.0.0.1";

	if (inet_pton(AF_INET, addressTemp.c_str(), &_serv_addr.sin_addr) <= 0){
		std::cerr << "Invalid address or address not suported" << std::endl;
		close(_socket);
		return ;
	}
	if (::connect(_socket, (struct sockaddr *)&_serv_addr, sizeof(_serv_addr)) < 0){
		std::cerr << "Connection to " << address << ":" << port << " failed" << std::endl;
		close(_socket);
		return ;
	}
	std::cout << "Connection made to " << address << ":" << port << std::endl;
	
}
void Client::disconnect(){
	std::cout << "Disconnecting ..." << std::endl;
	close(_socket);
}

void Client::defineAction(const Message::Type& messageType, const std::function<void(const Message& msg)>& action){
	_actions[messageType] = action;
}

void Client::send(const Message& message){
	std::string seriMsg = message.serialize();
	size_t msgSize = seriMsg.size();
	std::string sizeStr = std::to_string(msgSize) + "\r\n";

	if (::send(_socket, sizeStr.c_str(), sizeStr.size(), 0) == -1){
		std::cerr << "Failed to send message size: " << strerror(errno) << std::endl;
		return ;
	}
	if (::send(_socket, seriMsg.data(), msgSize, 0) == -1){
		std::cerr << "Failed to send message: " << strerror(errno) << std::endl;
		return ;
	}

	std::cout << "Message send with success" << std::endl;
	
}

bool Client::receiveMsg(const std::string &buffer){
	_buffer.append(buffer.c_str(), buffer.size());
	while (true){
		if (_msgSize == 0){
			auto size = _buffer.find("\r\n");
			if (size != std::string::npos){
				_msgSize = std::stoi(_buffer);
				_buffer.erase(0, (size + 2));// +2 for the \r\n
			}
		}
		if (_msgSize != 0 && _buffer.size() >= _msgSize){
			std::string msg = _buffer.substr(0, _msgSize);
			_buffer.erase(0, _msgSize);
			std::stringstream ss;
			ss << msg;
			_msgQueue.push(new Message(Message::deserialize(ss)));
			this->_msgSize = 0;
			continue;
		}
		break;
	}
	return true;
}

void Client::update(){
	std::cout << "Updating client ..." << std::endl;

	char buffer[1024];
	ssize_t bytesRead = ::recv(_socket, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead > 0){
		buffer[bytesRead] = '\0';
		std::string dataReceived(buffer, bytesRead);
		this->receiveMsg(dataReceived);
		auto msgQueue = this->getMessages();
		while (msgQueue->empty() == false){
			auto msg = msgQueue->front();
			msgQueue->pop();

			auto action = _actions.find(msg->getType());
			if (action != _actions.end())
				action->second(*msg);
			else
				std::cerr << "No action defined" << std::endl;
		}
	}
	else if (bytesRead == 0){
		std::cerr << "Server closed the connection" << std::endl;
		disconnect();
	}
	else
		std::cerr << "Failed to receive Message: " << strerror(errno) << std::endl;
}
