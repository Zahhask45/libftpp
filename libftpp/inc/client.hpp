#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "message.hpp"
#include <functional>
#include <unistd.h>
#include <cstring>
#include <queue>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>

class Client{
	public:
		Client()/*: _msgSize(0)*/{};
		void connect(const std::string& address, const size_t& port);
		void disconnect();
		void defineAction(const Message::Type& messageType, const std::function<void(const Message& msg)>& action);
		void send(const Message& message);
		void update();
	private:
		// size_t _msgSize;
		std::string _buffer;
		std::queue<Message *> _msgQueue;
		// Components
		int _socket;
		struct sockaddr_in _serv_addr;
		std::unordered_map<Message::Type, std::function<void(const Message&)>> _actions;
};

#endif
