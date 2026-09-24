# Fix these files only

The following are the remaining concrete fixes found in the current code.
Each section says exactly what to replace. `Observer`, `StateMachine`,
`ThreadSafeQueue`, `WorkerPool`, and `ThreadSafeIOStream` are not listed because
their current versions already contain the fixes discussed earlier.

---

## 1. PersistentWorker: replace both methods

File: `src/persistent_worker.cpp`

### Problem

`addTask()` now captures `name` and `jobToExecute` correctly, but task-map
access is still unsafe:

- `_toRemove[name]` can create entries unexpectedly;
- calling `removeTask()` with an unknown name creates a new `std::thread`
  entry through `_tasks[name]`;
- adding the same name twice creates a thread that is not stored correctly;
- `_tasks` is accessed without `_guard`.

### Replace `addTask()` with this

```cpp
void PersistentWorker::addTask(
    const std::string& name,
    const std::function<void()>& jobToExecute) {
    std::lock_guard<std::mutex> lock(_guard);

    if (_tasks.find(name) != _tasks.end())
        throw std::runtime_error("PersistentWorker: task already exists.");

    _toRemove[name] = false;
    _tasks.emplace(
        name,
        std::thread([this, name, jobToExecute]() {
            while (true) {
                {
                    std::lock_guard<std::mutex> guard(_guard);
                    if (_toRemove[name])
                        return;
                }

                jobToExecute();
            }
        })
    );
}
```

### Replace `removeTask()` with this

```cpp
void PersistentWorker::removeTask(const std::string& name) {
    std::thread task;

    {
        std::lock_guard<std::mutex> lock(_guard);

        auto taskIt = _tasks.find(name);
        if (taskIt == _tasks.end())
            throw std::runtime_error("PersistentWorker: unknown task.");

        _toRemove[name] = true;
        task = std::move(taskIt->second);
        _tasks.erase(taskIt);
    }

    if (task.joinable())
        task.join();

    {
        std::lock_guard<std::mutex> lock(_guard);
        _toRemove.erase(name);
    }
}
```

### Add these includes

At the top of `src/persistent_worker.cpp`:

```cpp
#include <stdexcept>
#include <utility>
```

### Replace the destructor

The destructor must not iterate over `_tasks` while another method can modify
the map. Replace it with:

```cpp
PersistentWorker::~PersistentWorker() {
    std::vector<std::thread> tasks;

    {
        std::lock_guard<std::mutex> lock(_guard);

        for (auto& entry : _toRemove)
            entry.second = true;

        for (auto& entry : _tasks)
            tasks.push_back(std::move(entry.second));

        _tasks.clear();
    }

    for (auto& task : tasks) {
        if (task.joinable())
            task.join();
    }
}
```

Add this include too:

```cpp
#include <vector>
```

Do not hold `_guard` while calling `jobToExecute()` or while joining a thread.

---

## 2. Client: send all bytes and handle zero-length payloads

File: `src/client.cpp`

### Problem

This code assumes one `send()` sends everything:

```cpp
if (::send(_sockfd, &size, sizeof(size), 0) < 0)
    throw SendingFailedException();
if (::send(_sockfd, data.c_str(), size, 0) < 0)
    throw SendingFailedException();
```

TCP is allowed to send only part of the requested buffer.

### Add this helper near the top of `src/client.cpp`

```cpp
namespace {
void sendAll(int socket, const char* data, size_t size) {
    size_t sent = 0;

    while (sent < size) {
        ssize_t result = ::send(socket, data + sent, size - sent, 0);
        if (result <= 0)
            throw Client::SendingFailedException();

        sent += static_cast<size_t>(result);
    }
}
}
```

### Replace the body of `Client::sendMsg()`

Keep the initial connection check, then use:

```cpp
void Client::sendMsg(const Message& message) {
    if (!_isConnected)
        throw NoConnectException();

    std::string data = message.serialize();
    size_t size = data.size();

    sendAll(_sockfd,
            reinterpret_cast<const char*>(&size),
            sizeof(size));

    if (!data.empty())
        sendAll(_sockfd, data.data(), data.size());
}
```

### Fix zero-length received messages

In `Client::receiveMsg()`, immediately after:

```cpp
clientBuf.data.resize(clientBuf.size, '\0');
```

add:

```cpp
if (clientBuf.size == 0) {
    clientBuf.state = ClientBuf::MESSAGE;
    clientBuf.totalBytes = 0;
}
```

Do not call `recv()` with a zero-length payload and interpret its result as a
disconnect.

---

## 3. Server: send all bytes and handle zero-length payloads

File: `src/server.cpp`

### Add this helper near the top of `src/server.cpp`

```cpp
namespace {
void sendAll(int socket, const char* data, size_t size) {
    size_t sent = 0;

    while (sent < size) {
        ssize_t result = ::send(socket, data + sent, size - sent, 0);
        if (result <= 0)
            throw Server::SendingFailedException();

        sent += static_cast<size_t>(result);
    }
}
}
```

### Replace `Server::sendMsg()`

```cpp
void Server::sendMsg(const Message& message, ClientID clientID) {
    if (!_isOn)
        throw NotStartedException();

    auto it = _clients.find(clientID);
    if (it == _clients.end())
        throw UnknownClientException();

    std::string data = message.serialize();
    size_t size = data.size();

    sendAll(it->second.sockfd,
            reinterpret_cast<const char*>(&size),
            sizeof(size));

    if (!data.empty())
        sendAll(it->second.sockfd, data.data(), data.size());
}
```

### Fix zero-length received messages

In `Server::receiveMsg()`, immediately after:

```cpp
client.data.resize(client.size, '\0');
```

add:

```cpp
if (client.size == 0) {
    client.state = Client::MESSAGE;
    client.totalBytes = 0;
}
```

---

## 4. Server: do not remove a message after a failed send

File: `src/server.cpp`

Current code:

```cpp
try{
    sendMsg(msgIt->second.front(), it->first);
} catch (const std::exception &e){}
msgIt->second.pop();
```

This loses the message even when `sendMsg()` fails.

Replace it with:

```cpp
try {
    sendMsg(msgIt->second.front(), it->first);
    msgIt->second.pop();
} catch (const std::exception&) {
    close(it->second.sockfd);
    _msgsToSend.erase(msgIt);
    it = _clients.erase(it);
}
```

Because erasing `it` invalidates the iterator, change the surrounding loop:

```cpp
for (auto it = _clients.begin(); it != _clients.end();) {
    if (!FD_ISSET(it->second.sockfd, &writefds)) {
        ++it;
        continue;
    }

    auto msgIt = _msgsToSend.find(it->first);
    if (msgIt == _msgsToSend.end() || msgIt->second.empty()) {
        ++it;
        continue;
    }

    try {
        sendMsg(msgIt->second.front(), it->first);
        msgIt->second.pop();
        ++it;
    } catch (const std::exception&) {
        close(it->second.sockfd);
        _msgsToSend.erase(msgIt);
        it = _clients.erase(it);
    }
}
```

---

## 5. Vector declarations: make the existing definitions const-correct

Files:

- `inc/ivector2.hpp`
- `src/ivector2.cpp`
- `inc/ivector3.hpp`
- `src/ivector3.cpp`

The current methods only accept non-const lvalue references. Replace the
`IVector2` declarations with:

```cpp
float length() const;
IVector2<float> normalize() const;
float dot(const IVector2& v2) const;
IVector2<TType> cross() const;

IVector2<TType> operator+(const IVector2& v2) const;
IVector2<TType> operator-(const IVector2& v2) const;
IVector2<TType> operator*(const IVector2& v2) const;
IVector2<TType> operator/(const IVector2& v2) const;
bool operator==(const IVector2& v2) const;
bool operator!=(const IVector2& v2) const;
```

Then make the matching changes in every `ivector2.cpp` definition:

```cpp
float IVector2<TType>::dot(const IVector2<TType>& v2) const
```

and similarly add `const` to the definitions of `normalize`, `cross`,
`operator+`, `operator-`, `operator*`, `operator/`, `operator==`, and
`operator!=`.

For `IVector3`, use:

```cpp
float length() const;
IVector3<float> normalize() const;
float dot(const IVector3& v2) const;
IVector3<TType> cross(const IVector3& v2) const;

IVector3<TType> operator+(const IVector3& v2) const;
IVector3<TType> operator-(const IVector3& v2) const;
IVector3<TType> operator*(const IVector3& v2) const;
IVector3<TType> operator/(const IVector3& v2) const;
bool operator==(const IVector3& v2) const;
bool operator!=(const IVector3& v2) const;
```

Apply the same `const` changes to `src/ivector3.cpp`.

Keep the zero-length checks already present in `normalize()`.

---

## 6. Vector templates: choose one of these two options

The definitions are in `.cpp` files, so currently only these types are
available:

```cpp
template struct IVector2<int>;
template struct IVector2<float>;
template struct IVector3<int>;
template struct IVector3<float>;
```

If supporting only `int` and `float` is intentional, leave the explicit
instantiations and do nothing.

If the class must work for any `TType`, move the template definitions from
the two `.cpp` files into their corresponding `.hpp` files. Do not keep both
the header definitions and the `.cpp` definitions, or you may get duplicate
definitions.

---

## 7. Makefile typo

File: `Makefile`

Replace:

```make
SRC = $(addpredix $(SRC_DIR), $(SRCS))
```

with:

```make
SRC = $(addprefix $(SRC_DIR), $(SRCS))
```

This variable is currently unused, so removing the line is also valid.

---

## Check after editing

Run:

```bash
make fclean
make
```

Then run a template check. This must compile without errors:

```cpp
#include "libftpp.hpp"

int main() {
    Observer<int> observer;
    observer.subscribe(1, [] {});
    observer.notify(1);

    StateMachine<int> machine;
    machine.addState(0);
    machine.addState(1);
    machine.addAction(0, [] {});
    machine.addTransition(0, 1, [] {});
    machine.transitionTo(1);

    ThreadSafeQueue<int> queue;
    queue.push_back(3);
    return queue.pop_front() != 3;
}
```

Compile it with:

```bash
c++ -std=c++17 -Wall -Wextra -Werror -pedantic \
    -I inc mandatory_smoke.cpp libftpp.a -pthread -o mandatory_smoke
./mandatory_smoke
```

---

# Bonus: Timer and Chronometer

These are not required for the mandatory mark. The PDF describes them as:

- **Timer:** stores a duration and reports whether that duration has elapsed.
- **Chronometer:** measures how much time has elapsed.

Use `std::chrono::steady_clock`, not `std::time()` or
`std::chrono::system_clock`. `steady_clock` is monotonic, so changing the
computer's wall clock cannot make a timer go backwards.

## Timer behavior

A sensible Timer API is:

```cpp
class Timer {
public:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    explicit Timer(const Duration& duration);

    void setDuration(const Duration& duration);
    void start();
    void reset();
    bool timedOut() const;
    bool isRunning() const;
};
```

Expected behavior:

```cpp
Timer timer(std::chrono::milliseconds(100));

timer.timedOut(); // false before start
timer.start();
timer.timedOut(); // normally false immediately

std::this_thread::sleep_for(std::chrono::milliseconds(120));
timer.timedOut(); // true

timer.reset();
timer.timedOut(); // false again
```

`timedOut()` should compare:

```cpp
Clock::now() - _startedAt >= _duration
```

Decide and document how zero or negative durations behave. The simplest rule
is that a started timer with a zero duration is immediately timed out.

## Chronometer behavior

A sensible Chronometer API is:

```cpp
class Chronometer {
public:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    Chronometer();

    void start();
    void stop();
    void reset();
    bool isRunning() const;
    Duration elapsed() const;
};
```

Expected behavior:

```cpp
Chronometer chrono;

chrono.elapsed(); // zero
chrono.start();
std::this_thread::sleep_for(std::chrono::milliseconds(50));
chrono.stop();

// elapsed() is now approximately 50 ms
auto milliseconds =
    std::chrono::duration_cast<std::chrono::milliseconds>(
        chrono.elapsed()
    ).count();
```

The useful design choice is that `start()` resumes an already stopped
Chronometer, while `stop()` accumulates the current interval. `reset()` clears
the accumulated duration and stops it.

For the bonus evaluation, add a main that checks:

1. a timer is not timed out before it starts;
2. a timer times out after its duration;
3. a reset timer is no longer timed out;
4. a chronometer accumulates one start/stop interval;
5. a chronometer accumulates two intervals;
6. `reset()` returns elapsed time to zero.

---

# Remaining mandatory hardening

These are not currently preventing the library from building or from running
the supplied mains. They are the remaining areas most likely to be questioned
during a detailed peer evaluation.

## A. PersistentWorker: protect `_tasks` consistently

File: `src/persistent_worker.cpp`

The task thread only needs `_guard` to read its stop flag. The public methods
also access `_tasks`, so do not read or modify `_tasks` without the same mutex.

The safe pattern is:

```cpp
std::thread task;

{
    std::lock_guard<std::mutex> lock(_guard);
    auto it = _tasks.find(name);
    if (it == _tasks.end())
        throw std::runtime_error("PersistentWorker: unknown task.");

    _toRemove[name] = true;
    task = std::move(it->second);
    _tasks.erase(it);
}

if (task.joinable())
    task.join();
```

Do not write this:

```cpp
auto& task = _tasks[name];
```

`operator[]` creates a new entry when the name does not exist.

Also reject duplicate names before creating a thread. A duplicate name must
not create a running thread that is absent from `_tasks`.

## B. Client and Server: reset stop state when reusing an object

Files:

- `src/client.cpp`
- `src/server.cpp`

Both classes have a `_shouldEnd` flag initialized only once:

```cpp
std::atomic<bool> _shouldEnd{false};
```

Before starting a new connection/server thread, explicitly reset it:

```cpp
_shouldEnd = false;
```

Add this in `Client::connect()` before starting `_receiver`, and in
`Server::start()` before starting `_receiver`. Otherwise, after one disconnect
or receive failure, reusing the same object can make the next receiver stop
immediately.

## C. Client and Server: close failure paths completely

Files:

- `src/client.cpp`
- `src/server.cpp`

When `connect()`, `bind()`, `listen()`, or non-blocking setup fails, close the
socket and reset its descriptor:

```cpp
close(_sockfd);
_sockfd = -1;
```

Do this on every failure path, not only on some of them. This prevents leaked
file descriptors and makes a retry possible.

For server shutdown, close each connected client socket and clear the client
maps after the receiver thread has stopped:

```cpp
for (const auto& entry : _clients)
    close(entry.second.sockfd);

_clients.clear();
_msgsToSend.clear();
_msgs.clear();
```

## D. Client and Server: validate received payload sizes

Files:

- `src/client.cpp`
- `src/server.cpp`

The size received from the peer is used directly in:

```cpp
data.resize(size, '\0');
```

Add a maximum message size before allocating. For example:

```cpp
static const size_t maximumMessageSize = 16 * 1024 * 1024;

if (clientBuf.size > maximumMessageSize) {
    _shouldEnd = true;
    return;
}
```

Use the equivalent server-side check. This prevents a corrupted or malicious
size header from requesting an unreasonable allocation.

## E. Client and Server: handle non-blocking `send()` correctly

The current `sendAll()` loop handles partial writes, but on a non-blocking
socket `send()` can return `-1` with `errno == EAGAIN` or `EWOULDBLOCK`.
Treating that immediately as a permanent failure can drop a valid message.

The robust solution is to keep unsent bytes in a per-connection output buffer
and retry when `select()` reports the socket writable. If keeping the current
simple design, at least distinguish retryable errors:

```cpp
if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    continue;
if (result <= 0)
    throw SendingFailedException();
```

Do not busy-loop forever; a proper output queue driven by writable events is
the better final design.

## F. Vectors: avoid integer overflow before converting to float

Files:

- `src/ivector2.cpp`
- `src/ivector3.cpp`

This expression can overflow for large integral vectors before `sqrt()` sees
it:

```cpp
sqrt(static_cast<float>(x * x + y * y))
```

Convert each coordinate before multiplication:

```cpp
float fx = static_cast<float>(x);
float fy = static_cast<float>(y);
return std::sqrt(fx * fx + fy * fy);
```

Do the same for `IVector3`. This is an improvement for large integer
coordinates; it does not change normal test results.

## G. DataBuffer: make failed string reads non-destructive

File: `src/data_buffer.cpp`

The current string deserializer first consumes the length and only then checks
whether all string bytes exist. If the payload is truncated, the buffer is
partially consumed.

Check both the length field and the complete string size before erasing
anything:

```cpp
void DataBuffer::deserialize(std::string& value) {
    size_t size;

    if (buffer.size() < sizeof(size))
        throw NotEnoughBytesException();

    std::memcpy(&size, buffer.data(), sizeof(size));
    if (buffer.size() - sizeof(size) < size)
        throw NotEnoughBytesException();

    const char* bytes = reinterpret_cast<const char*>(
        buffer.data() + sizeof(size)
    );
    value.assign(bytes, size);
    buffer.erase(buffer.begin(),
                 buffer.begin() + sizeof(size) + size);
}
```

This leaves the buffer unchanged when the read fails.
