#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "message.hpp"
#include <functional>
#include <queue>

class Client{
	private:
		void connect(const std::string& address, const size_t& port);
		void disconnect();
		void defineAction(const Message::Type& messageType, const std::function<void(const Message& msg)>& action);
		void send(const Message& message);
		void update();
	public:
		size_t _msgSize;
		std::string _buffer;
		std::queue<Message *> _msgQueue;
};

#endif
