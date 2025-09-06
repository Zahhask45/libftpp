#ifndef SERVER_HPP
#define SERVER_HPP

#include "message.hpp"
#include <functional>
#include <unordered_map>
#include <map>
#include <vector>
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>
#include <exception>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

class Server{
	public:
		using ClientID = long long;
		using Action = std::function<void(ClientID& clientID, Message& msg)>;

		Server();
		~Server();

		void start(const size_t &p_port);

		void defineAction(const Message::Type &messageType, const Action &action);
		
		void sendTo(const Message &message, ClientID clientID);
		void sendToArray(const Message &message, std::vector<ClientID> clientIDs);
		void sendToAll(const Message &message);

		void update();

	private:
		struct Client{
			enum State{
				NOSIZE,
				SIZE,
				MESSAGE
			};
			int sockfd;
			State state{State::NOSIZE};
			size_t size;
			std::string data;
			ssize_t bytesRead;
			size_t totalBytes = 0;
		};
	
		int _sockfd;

		std::map<ClientID, Client> _clients;
		std::vector<std::pair<ClientID, Message>> _msgs;
		std::map<ClientID, std::queue<Message>> _msgsToSend;
		std::unordered_map<Message::Type, Action> _actions;
		
		std::atomic<bool> _isOn;
		std::atomic<bool> _shouldEnd{false};
		std::thread _receiver;
		std::mutex _mtx;

	private:
		void stop();
		void receiveMsgs();
		void acceptConnection();
		std::map<ClientID, Client>::iterator receiveMsg(std::map<ClientID, Client>::iterator it);
		void sendMsg(const Message &message, ClientID clientID);

	public:
		//Exceptions
		class AlreadyStartedException : public std::exception {
			const char *what() const noexcept {return "Server: Already Started.";};
		};
		class NotStartedException : public std::exception {
			const char *what() const noexcept {return "Server: Not Started.";};
		};

		class FailedToStartException : public std::runtime_error {
			public:
				explicit FailedToStartException(const std::string &msg) :
					runtime_error("Server: " + msg + "."){};
		};
		class UnknownClientException : public std::runtime_error {
			public:
				explicit UnknownClientException() :
					runtime_error("Server: Unknown Client."){};
		};
		class SendingFailedException : public std::runtime_error {
			public:
				explicit SendingFailedException() :
					runtime_error("Server: Failed to send Message."){};
		};
		class BatchSendFailedException : public std::runtime_error {
			public:
				explicit BatchSendFailedException() :
					runtime_error("Server: Failed to Batch send all Messages."){};
		};
		
};

#endif
