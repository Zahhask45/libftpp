# Mandatory fixes still needed

This list contains only the parts that still need correction. The current
library builds with `make`, but the template smoke test exposes errors that
the Makefile does not instantiate.

## 1. Observer — compilation and callback storage

File: `inc/observer.hpp`

Current problems:

```cpp
auto it = event.find(event);
```

`event` is the function argument. The map is named `events`.

Also, this is invalid:

```cpp
std::vector<const std::function<void()>>
```

Use callback values, not pointers or `const` value types:

```cpp
template<typename TEvent>
class Observer {
public:
    void subscribe(const TEvent& event,
                   const std::function<void()>& callback) {
        _events[event].push_back(callback);
    }

    void notify(const TEvent& event) {
        auto it = _events.find(event);
        if (it == _events.end())
            return;

        for (const auto& callback : it->second)
            callback();
    }

private:
    std::unordered_map<TEvent, std::vector<std::function<void()>>> _events;
};
```

Do not store `&callback`; the argument may no longer exist when `notify()` is
called.

---

## 2. StateMachine — compilation and dangling callbacks

File: `inc/state_machine.hpp`

Replace both callback containers:

```cpp
std::vector<const std::function<void()>>
```

with:

```cpp
std::vector<std::function<void()>>
```

Store copies instead of addresses:

```cpp
void addAction(const TState& state,
               const std::function<void()>& callback) {
    _stateActions[state].push_back(callback);
}

void addTransition(const TState& startState,
                   const TState& finalState,
                   const std::function<void()>& callback) {
    _transitions[{startState, finalState}].push_back(callback);
}
```

The current code uses `&lambda`, which becomes dangling after the method
returns. Keep the existing value-based `_currentState` design and the existing
exceptions for missing states/transitions.

---

## 3. ThreadSafeQueue — include guard

File: `inc/thread_safe_queue.hpp`

The implementation is otherwise acceptable for the required queue API, but
the include guard should match the file and avoid generic/mistyped names:

```cpp
#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP
```

Keep the `mutable` mutex because `empty()` is `const`:

```cpp
mutable std::mutex _mutex;
```

---

## 4. ThreadSafeIOStream — one mutex for all threads

Files:

- `inc/thread_safe_iostream.hpp`
- `src/thread_safe_iostream.cpp`

`threadSafeCout` is thread-local, so a normal member mutex gives every thread
its own mutex. That does not synchronize output between threads.

Change the member mutex to a static shared mutex:

```cpp
private:
    static std::mutex _outputMutex;
    std::string _prefix;
    std::ostringstream _buffer;
```

Define it once:

```cpp
std::mutex ThreadSafeIOStream::_outputMutex;
thread_local ThreadSafeIOStream threadSafeCout;
```

Use `_outputMutex` in `setPrefix()`, input operations, and the manipulator
overload:

```cpp
ThreadSafeIOStream&
ThreadSafeIOStream::operator<<(
    std::ostream& (*manipulator)(std::ostream&)) {
    std::lock_guard<std::mutex> lock(_outputMutex);
    std::cout << _prefix << _buffer.str();
    manipulator(std::cout);
    _buffer.str("");
    _buffer.clear();
    return *this;
}
```

The buffer and prefix can remain per-thread; the actual I/O mutex must be
shared.

---

## 5. WorkerPool — interface and locking

File: `inc/worker_pool.hpp`

Make the interface destructor virtual:

```cpp
class IJobs {
public:
    virtual ~IJobs() = default;
    virtual void addJob(const std::function<void()>& jobToExecute) = 0;
};
```

File: `src/worker_pool.cpp`

Do not hold `_queueMutex` while executing user code. Pop the job, release the
lock, then call the job:

```cpp
std::function<void()> job;

{
    std::unique_lock<std::mutex> lock(_queueMutex);
    _condition.wait(lock, [&]() {
        return _stopthat || !_queue.empty();
    });

    if (_stopthat && _queue.empty())
        return;

    job = _queue.pop_front();
}

job();
```

Choose whether destruction drains or discards queued jobs. The code above
drains them before workers exit. Do not catch an exception and silently lose
the job without deciding what the worker-pool policy should be.

---

## 6. PersistentWorker — dangling captures and task lookup

File: `src/persistent_worker.cpp`

This is unsafe:

```cpp
std::thread([&]() {
    ...
    name
    jobToExecute()
})
```

`name` and `jobToExecute` are parameters of `addTask()` and the thread may
outlive that function. Capture them by value:

```cpp
std::thread([this, name, jobToExecute]() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(_guard);
            auto it = _toRemove.find(name);
            if (it != _toRemove.end() && it->second)
                return;
        }

        jobToExecute();
    }
})
```

Also fix task management:

- lock accesses to `_tasks` and `_toRemove`;
- reject duplicate task names;
- do not use `_tasks[name]` in `removeTask()`, because it creates a task if
  the name is unknown;
- throw or otherwise report removal of an unknown task;
- do not hold `_guard` while executing the user task.

---

## 7. Network receive code — incorrect byte pointer arithmetic

Files:

- `src/client.cpp`
- `src/server.cpp`

Do not use:

```cpp
&clientBuf.size + clientBuf.totalBytes
```

or:

```cpp
&client.size + client.totalBytes
```

That advances by `sizeof(size_t)` units instead of bytes. Use:

```cpp
char* sizeBytes =
    reinterpret_cast<char*>(&clientBuf.size) + clientBuf.totalBytes;
```

For the server, use the equivalent `client` expression.

---

## 8. Network send code — handle partial sends

Files:

- `src/client.cpp`
- `src/server.cpp`

One TCP `send()` call is not guaranteed to send the complete buffer. Replace
the direct calls with a loop:

```cpp
void sendAll(int socket, const char* data, size_t size) {
    size_t sent = 0;
    while (sent < size) {
        ssize_t result = ::send(socket, data + sent, size - sent, 0);
        if (result <= 0)
            throw SendingFailedException();
        sent += static_cast<size_t>(result);
    }
}
```

Use it for both the size header and serialized message data.

Do not remove a queued message after a failed or partial send unless the
message was fully transmitted.

---

## 9. Network shutdown — wake receiver threads

Files:

- `src/client.cpp`
- `src/server.cpp`

Both receiver threads can block inside `select()`. Changing an atomic flag
does not wake `select()`, so `disconnect()` or `stop()` can hang while joining.

The shutdown path must wake the socket before joining:

1. mark the object as stopping;
2. call `shutdown(socket, SHUT_RDWR)` where appropriate;
3. close or otherwise wake the socket/select loop;
4. join the receiver thread;
5. clear the descriptor.

Make sure another thread cannot use the descriptor after it is closed.

---

## 10. Vector API — const correctness and templates

Files:

- `inc/ivector2.hpp`
- `src/ivector2.cpp`
- `inc/ivector3.hpp`
- `src/ivector3.cpp`

Change input vector parameters to const references and mark read-only methods
const. For example:

```cpp
float dot(const IVector2& other) const;
IVector2<TType> operator+(const IVector2& other) const;
bool operator==(const IVector2& other) const;
```

Apply the same pattern to `IVector3`, including `cross()`.

Keep the zero-length guard in `normalize()`, and keep:

```cpp
return !(*this == other);
```

for `operator!=`.

The template definitions are currently in `.cpp` files, so only the explicit
`int` and `float` instantiations work. For a genuinely generic templated API,
move the definitions into the headers. Otherwise, explicitly limit and
document the supported types.

---

## 11. Direct includes and Makefile typo

Add the headers required directly by each file:

```cpp
#include <utility>
```

is needed where `std::move` or `std::forward` is used, especially
`inc/singleton.hpp` and `src/thread.cpp`.

In `Makefile`, this variable is misspelled and unused:

```make
SRC = $(addpredix $(SRC_DIR), $(SRCS))
```

Either remove it or correct it:

```make
SRC = $(addprefix $(SRC_DIR), $(SRCS))
```

---

## Validation after editing

Run the normal build:

```bash
make fclean
make
```

Then instantiate the templates, because `make` does not test all template
methods:

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
