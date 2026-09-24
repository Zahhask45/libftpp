#include "server.hpp"
#include "thread_safe_iostream.hpp"

Server::Server(): _sockfd(-1), _isOn(false){}

Server::~Server(){
	stop();
}

namespace{
	const size_t maximumMessageSize = 16 * 1024 * 1024;

	void sendAll(int socket, const char* data, size_t size) {
		size_t sent = 0;

		while (sent < size) {
			ssize_t result = ::send(socket, data + sent, size - sent, 0);
			if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
				continue ;
			if (result <= 0)
				throw Server::SendingFailedException();

			sent += static_cast<size_t>(result);
		}
	}
}


void Server::start(const size_t &p_port){
	if (_isOn)
		throw AlreadyStartedException();

	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockfd < 0)
		throw FailedToStartException("Failed to Start Socket");

	{
		const int enable = 1;
		setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
		setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int));
	}

	int flags = fcntl(_sockfd, F_GETFL, 0);
	if (flags == -1 || fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK) == -1){
		close(_sockfd);
		_sockfd = -1;
		throw FailedToStartException("Failed to set the non-block mode.");
	}

	sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	servAddr.sin_port = htons(static_cast<uint16_t>(p_port));

	if (bind(_sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0){
		close(_sockfd);
		_sockfd = -1;
		throw FailedToStartException("Failed to bind socket.");
	}

	if (listen(_sockfd, 5) < 0){
		close(_sockfd);
		_sockfd = -1;
		throw FailedToStartException("Failed to listen to the socket.");
	}

	_shouldEnd = false;
	_isOn = true;
	_receiver = std::thread(&Server::receiveMsgs, this);
}

void Server::stop(){
	if (!_isOn)
		return;

	_isOn = false;
	if (_receiver.joinable())
		_receiver.join();

	for (const auto &[clientID, client] : _clients){
		close(client.sockfd);
	}

	_clients.clear();
	_msgsToSend.clear();
	_msgs.clear();

	close(_sockfd);
	_sockfd = -1;
}

void Server::defineAction(const Message::Type &messageType, const Action &action){
	std::lock_guard<std::mutex> lock(_mtx);
	_actions[messageType] = action;
}

void Server::sendTo(const Message &message, ClientID clientID){
	if (!_isOn)
		throw NotStartedException();

	std::lock_guard<std::mutex> lock(_mtx);
	auto it = _msgsToSend.find(clientID);
	if (it == _msgsToSend.end())
		throw UnknownClientException();

	it->second.push(message);
}

void Server::sendToArray(const Message &message, std::vector<ClientID> clientIDs){
	if (!_isOn)
		throw NotStartedException();

	bool error = false;

	for (ClientID clientID : clientIDs){
		try {
			sendTo(message, clientID);
		} catch (const std::exception &e){
			error = true;
		}
	}
	if (error)
		throw BatchSendFailedException();
}

void Server::sendToAll(const Message &message){
	if (!_isOn)
		throw NotStartedException();

	bool error = false;

	for (const auto &client : _clients){
		try{
			sendTo(message, client.first);
		} catch (const std::exception &e){
			error = true;
		}
	}

	if (error)
		throw BatchSendFailedException();
}

void Server::update(){
	if (!_isOn)
		throw NotStartedException();

	if (_shouldEnd){
		stop();
		throw NotStartedException();
	}

	std::vector<std::pair<ClientID, Message>> procList;
	std::unordered_map<Message::Type, Action> actionsList;

	{
		std::lock_guard<std::mutex> lock(_mtx);
		procList.swap(_msgs);
		actionsList = _actions;
	}

	for (auto &[clientID, msg] : procList){
		auto it = actionsList.find(msg.type());
		if (it != actionsList.end() && it->second)
			it->second(clientID, msg);
	}
}

void Server::acceptConnection(){
	sockaddr_in  clientAddr{};
	socklen_t addrLen = sizeof(clientAddr);

	int clientSocket = accept(_sockfd, (struct sockaddr*)&clientAddr, &addrLen);
	if (clientSocket < 0)
		return ;

	ClientID newClientID = static_cast<ClientID>(clientSocket);
	Client newClient;
	newClient.sockfd = clientSocket;

	std::lock_guard<std::mutex> lock(_mtx);
	_clients[newClientID] = newClient;
	_msgsToSend[newClientID] = std::queue<Message>();
}

void Server::sendMsg(const Message &message, ClientID clientID){
	if (!_isOn)
		throw NotStartedException();

	auto it = _clients.find(clientID);
	if (it == _clients.end())
		throw UnknownClientException();

	std::string data = message.serialize();
	size_t size = data.size();

	sendAll(it->second.sockfd, reinterpret_cast<const char*>(&size), sizeof(size));

	if (!data.empty())
		sendAll(it->second.sockfd, data.data(), data.size());
}


std::map<Server::ClientID, Server::Client>::iterator Server::receiveMsg(std::map<Server::ClientID, Server::Client>::iterator it){
	Client& client = it->second;

	if (client.state == Client::NOSIZE){
		char* sizeBytes =
			reinterpret_cast<char*>(&client.size) + client.totalBytes;
		client.bytesRead = recv(client.sockfd,
			sizeBytes,
			sizeof(client.size) - client.totalBytes, 0);

		if (client.bytesRead <= 0){
			close(client.sockfd);
			_msgsToSend.erase(it->first);
			it = _clients.erase(it);
			return it;
		}

		client.totalBytes += client.bytesRead;
		if (client.totalBytes < sizeof(client.size))
			return ++it;

		client.state = Client::SIZE;
		client.totalBytes = 0;
		if (client.size > maximumMessageSize){
			close(client.sockfd);
			_msgsToSend.erase(it->first);
			return _clients.erase(it);
		}
		client.data.resize(client.size, '\0');
		if (client.size == 0){
			client.state = Client::MESSAGE;
			client.totalBytes = 0;
		}
	} else if (client.state == Client::SIZE){
		client.bytesRead = recv(client.sockfd,
			client.data.data() + client.totalBytes,
			client.size - client.totalBytes, 0);

		if (client.bytesRead <= 0){
			close(client.sockfd);
			_msgsToSend.erase(it->first);
			it = _clients.erase(it);
			return it;
		}

		client.totalBytes += client.bytesRead;
		if (client.totalBytes < client.size)
			return ++it;

		client.state = Client::MESSAGE;
		client.totalBytes = 0;
	}

	if (client.state == Client::MESSAGE){
		client.state = Client::NOSIZE;
		Message msg(0);
		try {
			msg.deserialize(client.data);
		} catch (const std::exception &e){
			return ++it;
		}

		auto t = _msgs.emplace_back(it->first, std::move(msg));
	}
	return ++it;
}


void Server::receiveMsgs(){
	fd_set readfds;
	fd_set writefds;

	while (_isOn){
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(_sockfd, &readfds);
		int max_fd = _sockfd;

		{
			std::lock_guard<std::mutex> lock(_mtx);
			for (const auto& client : _clients){
				FD_SET(client.second.sockfd, &readfds);
				FD_SET(client.second.sockfd, &writefds);
				if (client.second.sockfd > max_fd)
					max_fd = client.second.sockfd;
			}
		}

		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
		int act = select(max_fd + 1, &readfds, &writefds, nullptr, &timeout);
		if (act < 0)
			continue ;

		if (FD_ISSET(_sockfd, &readfds))
			acceptConnection();

		{
			std::lock_guard<std::mutex> lock(_mtx);
			for (auto it = _clients.begin(); it != _clients.end();){
				if (FD_ISSET(it->second.sockfd, &readfds)){
					it = receiveMsg(it);
					if (it == _clients.end())
						break;
				}else {
					it++;
				}
			}
		}

		{
			std::lock_guard<std::mutex> lock(_mtx);
			for (auto it = _clients.begin(); it != _clients.end();){
				if (!FD_ISSET(it->second.sockfd, &writefds)){
					++it;
					continue ;
				}

				auto msgIt = _msgsToSend.find(it->first);
				if (msgIt == _msgsToSend.end() || msgIt->second.empty()){
					++it;
					continue ;
				}

				try{
					sendMsg(msgIt->second.front(), it->first);
					msgIt->second.pop();
					++it;
				} catch (const std::exception&){
					close(it->second.sockfd);
					_msgsToSend.erase(msgIt);
					it = _clients.erase(it);
				}
			}
		}
	}
}
