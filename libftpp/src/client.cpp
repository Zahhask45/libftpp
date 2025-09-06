#include "client.hpp"

Client::Client(): _sockfd(-1), _isConnected(false){}

Client::~Client(){
	disconnect();
}

void Client::connect(const std::string &address, const size_t &port){
	if (_isConnected)
		throw AlreadyConnectException();

	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockfd < 0)
		throw ConnectionFailedException("Failed to create socket");

	int flags = fcntl(_sockfd, F_GETFL, 0);
	if (flags == -1 || fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw ConnectionFailedException("Failed to set non-block mode");

	sockaddr_in servAddr{};
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port);

	if (address == "localhost"){
		struct addrinfo hints{}, *res;
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		int status = getaddrinfo("localhost", nullptr, &hints, &res);
		if (status != 0)
			throw ConnectionFailedException("Failed to resolve localhost");

		servAddr.sin_addr = reinterpret_cast<sockaddr_in *>(res->ai_addr)->sin_addr;
		freeaddrinfo(res);
	}
	else if (inet_pton(AF_INET, address.c_str(), &servAddr.sin_addr) <= 0){
		close(_sockfd);
		throw ConnectionFailedException("Invalid address");
	}
	
	if (::connect(_sockfd, reinterpret_cast<sockaddr *>(&servAddr), sizeof(servAddr)) < 0){
		if (errno != EINPROGRESS){
			close(_sockfd); 
			throw ConnectionFailedException("Failed to connect");
		}
	}

	fd_set writefds;
	FD_ZERO(&writefds);
	FD_SET(_sockfd, &writefds);

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	int selectResult =  select(_sockfd + 1, nullptr, &writefds, nullptr, &timeout);
	if (selectResult == 0){
		close(_sockfd);
		throw ConnectionFailedException("Failed to connect(timeout)");
	} else if (selectResult < 0 || !FD_ISSET(_sockfd, &writefds)){
		close(_sockfd);
		throw ConnectionFailedException("Failed to connect(select error)");
	}

	int error = 0;
	socklen_t len = sizeof(error);
	if (getsockopt(_sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0){
		close(_sockfd);
		throw ConnectionFailedException("Failed to connect: " + std::string(strerror(error)));
	}

	_isConnected = true;
	_receiver = std::thread(&Client::receiveMsgs, this);
}

void Client::disconnect(){
	if (!_isConnected)
		return ;

	_isConnected = false;
	if (_receiver.joinable())
		_receiver.join();

	close(_sockfd);
	_sockfd = -1;
}

void Client::send(const Message &message){
	if (!_isConnected)
		throw NoConnectException();

	std::lock_guard<std::mutex> lock(_mtx);
	_msgsToSend.push(message);
}

void Client::defineAction(const Message::Type &messageType, const Action &action){
	std::lock_guard<std::mutex> lock(_mtx);
	_actions[messageType] = action;
	
}

void Client::update(){
	if (!_isConnected)
		throw NoConnectException();

	if (_shouldEnd){
		disconnect();
		throw NoConnectException();
	}

	std::vector<Message> procList;
	std::unordered_map<Message::Type, Action> actionList;

	{
		std::lock_guard<std::mutex> lock(_mtx);
		procList.swap(_msgs);
		actionList = _actions;
	}

	for (Message& msg : procList){
		auto it = actionList.find(msg.type());

		if (it != actionList.end() && it->second)
			it->second(msg);
	}
}

void Client::receiveMsg(ClientBuf &clientBuf){
	if (clientBuf.state == ClientBuf::NOSIZE){
		clientBuf.bytesRead = ::recv(_sockfd,
				&clientBuf.size + clientBuf.totalBytes,
				sizeof(clientBuf.size) - clientBuf.totalBytes,
				0);

		if (clientBuf.bytesRead <= 0){
			_shouldEnd = true;
			return ;
		}

		clientBuf.totalBytes += clientBuf.bytesRead;
		if (clientBuf.totalBytes < sizeof(clientBuf.size))
			return ;

		clientBuf.state = ClientBuf::SIZE;
		clientBuf.totalBytes = 0;
		clientBuf.data.resize(clientBuf.size, '\0');
	} else if (clientBuf.state == ClientBuf::SIZE){
		clientBuf.bytesRead = ::recv(_sockfd,
				clientBuf.data.data() + clientBuf.totalBytes,
				clientBuf.size - clientBuf.totalBytes,
				0);

		if (clientBuf.bytesRead <= 0){
			_shouldEnd = true;
			return ;
		}

		clientBuf.totalBytes += clientBuf.bytesRead;
		if (clientBuf.totalBytes < clientBuf.size)
			return ;

		clientBuf.state = ClientBuf::MESSAGE;
		clientBuf.totalBytes = 0;
	}
	if (clientBuf.state == ClientBuf::MESSAGE){
		clientBuf.state = ClientBuf::NOSIZE;
		Message msg(0);
		std::stringstream ss(clientBuf.data);
		msg.deserialize(ss);

		std::lock_guard<std::mutex> lock(_mtx);
		_msgs.push_back(std::move(msg));
	}
}

void Client::sendMsg(const Message &message){
	if (!_isConnected)
		throw NoConnectException();

	std::string data = message.serialize();
	size_t size = data.size();

	if (::send(_sockfd, &size, sizeof(size), 0) < 0)
		throw SendingFailedException();
	if (::send(_sockfd, data.c_str(), size, 0) < 0)
		throw SendingFailedException();
}


void Client::receiveMsgs(){
	ClientBuf clientBuf;

	fd_set  readfds;
	fd_set  writefds;

	while (_isConnected){
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(_sockfd, &readfds);
		FD_SET(_sockfd, &writefds);
		int max_fd = _sockfd;

		int act = select(max_fd + 1, &readfds, &writefds, nullptr, nullptr);
		if (act < 0)
			continue;
			
		if (!_shouldEnd && FD_ISSET(_sockfd, &readfds))
			receiveMsg(clientBuf);

		if (!_shouldEnd && FD_ISSET(_sockfd, &writefds)){
			Message msg(0);
			bool send = false;
			{
				std::lock_guard<std::mutex> lock(_mtx);
				if (!_msgsToSend.empty()){
					msg = _msgsToSend.front();
					_msgsToSend.pop();
					send = true;
				}
			}
			if (send)
				sendMsg(msg);
		}
	}
}
