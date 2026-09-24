# Remaining fixes

The following issues are already fixed in the current code and should not be
changed again:

- `PersistentWorker` task-map locking and value captures;
- client/server partial-send handling;
- client/server zero-length payload handling;
- server failed-send queue handling;
- vector method const-correctness;
- the Makefile `addprefix` typo.

Only the items below remain.

## 1. Reset client/server state before reuse

### Client

File: `src/client.cpp`
Function: `Client::connect()`

Find the end of the successful connection path:

```cpp
_isConnected = true;
_receiver = std::thread(&Client::receiveMsgs, this);
```

Insert this directly before `_isConnected = true;`:

```cpp
_shouldEnd = false;
```

### Server

File: `src/server.cpp`
Function: `Server::start()`

Find:

```cpp
_isOn = true;
_receiver = std::thread(&Server::receiveMsgs, this);
```

Insert this directly before `_isOn = true;`:

```cpp
_shouldEnd = false;
```

This is needed when an object is reused after a previous receive failure.

## 2. Finish socket cleanup and clear server state

### Client failure paths

File: `src/client.cpp`
Function: `Client::connect()`

After every failure where `_sockfd` was successfully created, make sure the
branch contains both lines:

```cpp
close(_sockfd);
_sockfd = -1;
```

Check these exact failure cases:

- failed `fcntl()`;
- failed `getaddrinfo()`;
- invalid `inet_pton()`;
- failed `connect()`;
- connection timeout;
- failed `select()`;
- failed `getsockopt()`.

For example, change:

```cpp
close(_sockfd);
throw ConnectionFailedException("Failed to connect");
```

to:

```cpp
close(_sockfd);
_sockfd = -1;
throw ConnectionFailedException("Failed to connect");
```

### Server failure paths

File: `src/server.cpp`
Function: `Server::start()`

After failures from `fcntl()`, `bind()`, and `listen()`, ensure the branch
contains:

```cpp
close(_sockfd);
_sockfd = -1;
throw FailedToStartException("...");
```

### Server shutdown state

File: `src/server.cpp`
Function: `Server::stop()`

After this existing loop closes the connected client sockets:

```cpp
for (const auto &[clientID, client] : _clients) {
	close(client.sockfd);
}
```

insert:

```cpp
_clients.clear();
_msgsToSend.clear();
_msgs.clear();
```

Keep those lines before:

```cpp
close(_sockfd);
_sockfd = -1;
```

This prevents old clients and queued messages surviving a later `start()`.

## 3. Reject oversized network payloads

### Add the limit

Files: `src/client.cpp`, `src/server.cpp`

Near the top of each file, after the includes and before the class method
definitions, add an anonymous namespace constant:

```cpp
namespace {
	const size_t maximumMessageSize = 16 * 1024 * 1024;
}
```

If the file already has an anonymous namespace containing `sendAll()`, put the
constant inside that existing namespace instead of creating another one.

### Client check

File: `src/client.cpp`
Function: `Client::receiveMsg(ClientBuf& clientBuf)`

Find:

```cpp
clientBuf.data.resize(clientBuf.size, '\0');
```

Insert directly before it:

```cpp
if (clientBuf.size > maximumMessageSize) {
	_shouldEnd = true;
	return;
}
```

### Server check

File: `src/server.cpp`
Function: `Server::receiveMsg(...)`

Find:

```cpp
client.data.resize(client.size, '\0');
```

Insert directly before it:

```cpp
if (client.size > maximumMessageSize) {
	close(client.sockfd);
	return _clients.erase(it);
}
```

The caller already assigns the iterator returned by `receiveMsg()`.

## 4. Handle non-blocking `send()` errors

Files: `src/client.cpp`, `src/server.cpp`

Function in both files: the anonymous-namespace `sendAll(...)` helper.

Find:

```cpp
if (result <= 0)
	throw Client::SendingFailedException();
```

in `src/client.cpp`, or the equivalent server exception in `src/server.cpp`.

The proper solution is an output queue that retains unsent bytes and retries
them when `select()` reports the socket writable. If keeping the current
design, at least distinguish retryable errors:

```cpp
if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
	continue;
}
if (result <= 0)
	throw Client::SendingFailedException();
```

Use `Server::SendingFailedException()` in `src/server.cpp`.

Do not use this minimal `continue` version if you replace the send path with a
proper output queue. A busy retry loop is not the ideal final implementation.

## 5. Avoid integer overflow in vector length calculations

### IVector2

File: `src/ivector2.cpp`
Function: `IVector2<TType>::length() const`

Replace the current body:

```cpp
return (sqrt(static_cast<float>(x*x + y*y)));
```

with:

```cpp
float fx = static_cast<float>(x);
float fy = static_cast<float>(y);
return std::sqrt(fx * fx + fy * fy);
```

### IVector3

File: `src/ivector3.cpp`
Function: `IVector3<TType>::length() const`

Replace:

```cpp
return (sqrt(static_cast<float>(x*x + y*y + z*z)));
```

with:

```cpp
float fx = static_cast<float>(x);
float fy = static_cast<float>(y);
float fz = static_cast<float>(z);
return std::sqrt(fx * fx + fy * fy + fz * fz);
```

## 6. Make failed `DataBuffer` string reads non-destructive

File: `src/data_buffer.cpp`
Function: `DataBuffer::deserialize(std::string& str)`

Replace the complete existing string-deserialization method:

```cpp
void DataBuffer::deserialize(std::string& str) {
	size_t size;

	if (buffer.size() < sizeof(size))
		throw NotEnoughBytesException();

	std::memcpy(&size, buffer.data(), sizeof(size));
	if (buffer.size() - sizeof(size) < size)
		throw NotEnoughBytesException();

	const char* bytes = reinterpret_cast<const char*>(
		buffer.data() + sizeof(size)
	);
	str.assign(bytes, size);
	buffer.erase(
		buffer.begin(),
		buffer.begin() + sizeof(size) + size
	);
}
```

At the top of `src/data_buffer.cpp`, directly after:

```cpp
#include "data_buffer.hpp"
```

add this if it is not already provided by the header:

```cpp
#include <cstring>
```

## Validation

After editing:

```bash
make fclean
make
```

Then recompile and run the supplied mains, especially the client/server
mains. Do not apply all six changes at once if you want easier debugging;
apply one section, rebuild, and test.
