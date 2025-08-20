#include "server.hpp"

void Server::start(const size_t& p_port){
	if ((_sockFd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		std::cerr << "Socket creation failed." << std::endl;
		return ;
	}
	if (setsockopt(_sockFd, SOL_SOCKET, SO_REUSEADDR, &_opt, sizeof(_opt))){
		std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
		close(_sockFd);
		return ;
	}
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(p_port);

	if (bind(_sockFd, (struct sockaddr*)&_address, sizeof(_address)) < 0){
		std::cerr << "Failed to bind socket to port " << p_port << std::endl;
		close(_sockFd);
		return ;
	}
	if (listen(_sockFd, 42) < 0){
		std::cerr << "Failed to listen on port " << p_port << std::endl;
		close(_sockFd);
		return ;
	}
	std::cout << "Server started on localhost:" << p_port << std::endl;
}

void Server::defineAction(const Message::Type& messageType, const std::function<void(long long& clientID, const Message& msg)>& action){
	_actions[messageType] = action;
}

void Server::sendTo(const Message& message, long long clientID){
	auto it = _clientSockets.find(clientID);
	if (it == _clientSockets.end()){
		std::cerr << "Client ID: " << clientID << " not existent" << std::endl;
		return ;
	}
	int socket_fd = it->second->getSocket();
	std::string serimsg = message.serialize();
	std::string msgSize = std::to_string(serimsg.size()) + "\r\n";

	if (send(socket_fd, msgSize.c_str(), msgSize.size(), 0) == -1){
		std::cerr << "Failed to send Message size to client: " << clientID << std::endl;
		return ;
	}
	if (send(socket_fd, serimsg.c_str(), serimsg.size(), 0) == -1){
		std::cerr << "Failed to send Message to client: " << clientID << std::endl;
		return ;
	}
}
 void Server::sendToArray(const Message& message, std::vector<long long> clientIDs){
 	for (const auto& clientID :clientIDs)
 		sendTo(message, clientID);
 }

void Server::sendToAll(const Message& message){
	for (const auto& clientSocket : _clientSockets){
		sendTo(message, clientSocket.first);
	}
}

void Server::update(){
	fd_set readFds;
	FD_ZERO(&readFds);

	int maxFd = _sockFd;
	FD_SET(_sockFd, &readFds);

	for (const auto& clientSocket : _clientSockets){
		auto client = clientSocket.second;
		
		int socket_fd = client->getSocket();
		FD_SET(socket_fd, &readFds);
		if (socket_fd > maxFd)
			maxFd = socket_fd;
	}
	fd_set tmpReadFds = readFds;
	int activity = select(maxFd + 1, &tmpReadFds, nullptr, nullptr, nullptr);

	if (activity < 0 && errno != EINTR){
		std::cerr << "select() error: " << strerror(errno) << std::endl;
		return ;
	}

	if (FD_ISSET(_sockFd, &tmpReadFds)){
		int new_socket = accept(_sockFd, (struct sockaddr*)&_address, (socklen_t*)&_addrlen);
		if (new_socket < 0){
			std::cerr << "Failed to accept new connection: " << strerror(errno) << std::endl;
			return ;
		}

		long long clientID = new_socket;
		_clientSockets[clientID] = new Client(new_socket);
		std::cout << "New Client connected: ID->" << clientID << std::endl;
	}

	for (auto it = _clientSockets.begin(); it != _clientSockets.end(); ){
		long long clientID = it->first;
		int socket_fd  = it->second->getSocket();

		if (FD_ISSET(socket_fd, &readFds)){
			char buffer[1024] = {0};
			ssize_t bytesReceiv = recv(socket_fd, buffer, sizeof(buffer), 0);

			if (bytesReceiv <= 0) {
				if (bytesReceiv == 0){
					std::cout << "Client " << clientID << " disconnected" << std::endl;
				} else {
					std::cerr << "Recv failed for client " << clientID << ": " << strerror(errno) << std::endl;
				}
				close(socket_fd);
				delete it->second;
				it = _clientSockets.erase(it);
				continue ;
			}
			it->second->receiveMsg(buffer);
			auto msgQueue = it->second->getMessages();

			while (!msgQueue->empty()){
				auto msg = msgQueue->front();
				msgQueue->pop();
				auto action = _actions.find(msg->getType());
				if (action != _actions.end())
					action->second(clientID, *msg);
				else
					std::cerr << "No action defined for type: " << msg->getType() << std::endl;
			}
		}
		it++;
	}
	
}
