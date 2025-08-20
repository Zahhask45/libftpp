#ifndef SERVER_HPP
#define SERVER_HPP

#include "client.hpp"
#include "message.hpp"

class Server{
	public:
		void start(const size_t& p_port);
		void defineAction(const Message::Type& messageType, const std::function<void(long long& clientID, const Message& msg)>& action);
		void sendTo(const Message& message, long long clientID);
		void sendToArray(const Message& message, std::vector<long long> clientIDs);
		void sendToAll(const Message& message);
		void update();
	private:
		std::unordered_map<long long, Client *> _clientSockets;
		std::unordered_map<Message::Type, std::function<void(long long&, const Message&)>> _actions;
		// Server components
		int _sockFd;
		struct sockaddr_in _address;
		int _opt = 1;
		int _addrlen = sizeof(_address);
};

#endif
