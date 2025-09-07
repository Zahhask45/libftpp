#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "message.hpp"
#include <atomic>
#include <exception>
#include <functional>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <thread>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

class Client{
	public:
		using Action = std::function<void(Message &msg)>;

		Client();
		~Client();

		void connect(const std::string &address, const size_t &port);
		void disconnect();

		
		void send(const Message &message);
		void defineAction(const Message::Type &messageType, const Action &action);
		
		void update();

	private:
		struct ClientBuf{
			enum State{
				NOSIZE,
				SIZE,
				MESSAGE
			};
			State state;
			size_t size;
			std::string data;
			ssize_t bytesRead;
			size_t totalBytes = 0;
		};
		
		int _sockfd;
		std::unordered_map<Message::Type, Action> _actions;
		std::vector<Message> _msgs;
		std::queue<Message> _msgsToSend;
		std::atomic<bool> _isConnected;
		std::atomic<bool> _shouldEnd{false};
		std::thread _receiver;
		std::mutex _mtx;

	private:
		void receiveMsgs();
		void receiveMsg(ClientBuf &clientBuf);
		void sendMsg(const Message &message);

	public:
		// Exceptions
		class AlreadyConnectException : public std::exception {
			const char *what() const noexcept {return "Client already connected";};
		};
		class NoConnectException : public std::exception {
			const char *what() const noexcept {return "Client not connected";};
		};
		class ConnectionFailedException : public std::runtime_error{
			public:
				explicit ConnectionFailedException(const std::string &msg):
				runtime_error("Client: " + msg + "."){}
		};
		class SendingFailedException : public std::runtime_error {
			public:
				explicit SendingFailedException():
				runtime_error("Client: Failed to send Message."){}
		};
};

#endif
