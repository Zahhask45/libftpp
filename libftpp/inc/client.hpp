#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "message.hpp"
#include <atomic>
#include <exception>
#include <functional>
#include <unistd.h>
#include <cstring>
#include <queue>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>

class Client{
	public:
		using Action = std::function<void(const Message& msg)>;

		Client();
		~Client();

		void connect(const std::string& address, const size_t& port);
		void disconnect();

		
		void send(const Message& message);
		void defineAction(const Message::Type& messageType, Action& action);
		
		void update();

	private:
		int _sockfd;
		std::atomic<bool> _isConnected;
		std::unordered_map<Message::Type, Action> _actions;

	public:
		// Exceptions
		class AlreadyConnectExceptions : public std::exception {
			const char *what() const noexcept {return "Client already connected";};
		};
		class NoConnectExceptions : public std::exception {
			const char *what() const noexcept {return "Client not connected";};
		};
};

#endif
