# Mandatory-part repair guide

This guide is based on `en.subject.pdf` and the current project files. It is
written for manual editing: apply the fixes section by section, then run the
validation commands at the end.

The PDF requires:

- compilation with `c++ -Wall -Wextra -Werror` and at least C++11;
- `libftpp.a`;
- `inc/libftpp.hpp`;
- the section headers and classes listed in the mandatory part;
- no external libraries and no `printf`, `malloc`-family allocation, or
  `free`;
- independent headers with include guards.

The current Makefile builds successfully, but that does not instantiate every
template. Always run the template smoke test in the validation section too.

---

## 1. Observer

### Current problems

`inc/observer.hpp` currently has three issues:

1. `notify()` calls `event.find(event)`. `event` is the `TEvent` argument;
   the map is named `events`.
2. `std::vector<const std::function<void()>>` is invalid. A standard
   container cannot have a `const` value type.
3. The callbacks should be stored by value, not as pointers to the argument
   passed to `subscribe()`.

### Replace the implementation with

```cpp
#ifndef OBSERVER_HPP
#define OBSERVER_HPP

#include <functional>
#include <unordered_map>
#include <vector>

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

#endif
```

`TEvent` must be usable as a key in `std::unordered_map`, which means it needs
`operator==` and `std::hash<TEvent>`.

---

## 2. StateMachine

### Current problems

`inc/state_machine.hpp` has the same invalid callback container as Observer:

```cpp
std::vector<const std::function<void()>>
```

It also does this:

```cpp
_stateActions[state].push_back(&lambda);
_transitions[{startState, finalState}].push_back(&lambda);
```

`lambda` is a reference to the function argument. Its address becomes invalid
when `addAction()` or `addTransition()` returns.

### Required changes

Use callback values in both maps:

```cpp
std::unordered_map<
    TState,
    std::vector<std::function<void()>>
> _stateActions;

std::unordered_map<
    TransitionKey,
    std::vector<std::function<void()>>,
    TransitionKeyHash
> _transitions;
```

Then store copies:

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

The current value-based current-state design is fine:

```cpp
TState _currentState{};
bool _hasCurrentState = false;
```

Keep these checks:

- `transitionTo()` must throw if no current state exists;
- `transitionTo()` must throw if the requested transition is not registered;
- `update()` must throw if no current state exists.

When invoking callbacks, use:

```cpp
for (const auto& callback : it->second)
    callback();
```

Do not use raw pointers and do not restore the old raw `_curstate` design.

---

## 3. ThreadSafeQueue

`inc/thread_safe_queue.hpp` is close to correct. Keep the mutex around every
operation, including `empty()`:

```cpp
bool empty() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _queue.empty();
}
```

Because `empty()` is `const`, the mutex must be declared `mutable`:

```cpp
mutable std::mutex _mutex;
```

Fix the include guard spelling. It should be specific and consistent:

```cpp
#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP
```

The queue methods required by the PDF are:

- `push_back`;
- `push_front`;
- `pop_back`;
- `pop_front`.

The pop methods must throw when the queue is empty. The current implementation
does that.

---

## 4. Thread

The current `Thread` lifecycle checks are useful:

- reject `start()` when the thread is already joinable;
- make `stop()` harmless when the thread was never started;
- join a started thread.

Keep the implementation conceptually like this:

```cpp
void Thread::start() {
    if (_thread.joinable())
        throw std::runtime_error("Thread: already started");
    if (!_func)
        throw std::runtime_error("Thread: no function assigned");

    _thread = std::thread(&Thread::run, this);
}

void Thread::stop() {
    if (_thread.joinable())
        _thread.join();
}

void Thread::run() {
    threadSafeCout.setPrefix(_name);
    _func();
}
```

`src/thread.cpp` and `inc/thread.hpp` must use the same function name. If the
header declares `run()`, do not leave a `_trueStart()` definition in the
source.

Add `<utility>` in the source if using `std::move`.

---

## 5. Pool

The PDF says that the pool must:

- allocate a number of reusable objects;
- construct an acquired object with the arguments passed to `acquire()`;
- destroy the object on release;
- return the memory to the pool without deallocating it;
- let `Pool::Object` handle acquisition and release.

Do not remove shrinking. Shrinking is valid when only available slots are
removed.

### Why raw storage is needed

This combination is unsafe:

```cpp
std::make_unique<TType>();
new (obj) TType(args...);
```

The first line constructs a live `TType`; the second constructs another object
over it without first ending the original lifetime. A manually called
destructor then conflicts with the `unique_ptr` owner.

The current `inc/pool.hpp` uses a private aligned-storage `Slot`, which is a
reasonable internal solution. Verify that it has all of these properties:

1. `resize()` allocates slots, but does not construct `TType`.
2. `acquire()` constructs `TType` exactly once with placement `new`.
3. `Pool::Object::~Object()` calls `~TType()` and returns the slot.
4. `Object` is move-only.
5. Shrinking removes only entries from the available list.
6. Shrinking checks that enough slots are available before changing anything.
7. If construction throws, the slot is returned to the available list.

Important ownership rule: the `Pool` must outlive all its `Pool::Object`
instances.

The object wrapper should have this shape:

```cpp
Object(const Object&) = delete;
Object& operator=(const Object&) = delete;

Object(Object&& other) noexcept;
Object& operator=(Object&& other) noexcept;
```

Without move-only ownership, two wrappers could release the same slot.

Do not make a `noexcept` release function call an operation that can allocate
unless you accept the consequence. `std::vector::push_back()` can throw while
returning a slot. For a simple project implementation, this is usually left as
is, but do not claim it is unconditionally exception-safe.

---

## 6. DataBuffer

`DataBuffer` provides the templated `<<` and `>>` operators required by the
PDF. Keep the trivially-copyable restriction for primitive data:

```cpp
std::enable_if_t<std::is_trivially_copyable_v<TType>, void>
```

The string overload must serialize a length followed by the bytes.

Check these details manually:

- `deserialize()` must throw `NotEnoughBytesException` before reading beyond
  the buffer;
- string length must be validated before assigning the string;
- serialization must not use forbidden C allocation functions;
- the header must compile independently.

For stronger behavior, deserialize a string length without permanently
removing it until the complete string payload has been validated. Otherwise a
failed string read can leave the buffer partially consumed.

A minimal safe pattern is:

```cpp
void deserialize(std::string& value) {
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

---

## 7. Memento

The PDF requires:

- `Snapshot save()`;
- `void load(const Snapshot&)`;
- private save/load hooks implemented by the inheriting class.

The current `Memento` design is suitable:

```cpp
Snapshot Memento::save() const {
    Snapshot snapshot;
    _saveToSnapshot(snapshot);
    return snapshot;
}

void Memento::load(const Snapshot& snapshot) {
    Snapshot copy = snapshot;
    _loadFromSnapshot(copy);
}
```

The copy in `load()` is intentional because deserialization consumes bytes
from `DataBuffer`.

An inheriting class should implement:

```cpp
void _saveToSnapshot(Snapshot& snapshot) const override;
void _loadFromSnapshot(Snapshot& snapshot) override;
```

Do not define a destructor in `memento.cpp` if the header already says:

```cpp
virtual ~Memento() = default;
```

That creates a duplicate definition.

---

## 8. Singleton

The current singleton structure satisfies the required operations:

- `instance()` returns the object or throws if not instantiated;
- `instantiate()` forwards constructor arguments;
- a second instantiation throws.

Add the direct dependency:

```cpp
#include <utility>
```

because the implementation uses `std::forward`.

The target class must declare the singleton as a friend if its constructor is
private or protected, as shown in the subject example.

---

## 9. ThreadSafeIOStream

The PDF requires:

- `<<` and `>>`;
- `setPrefix()`;
- `prompt()`;
- thread-safe output;
- a thread-local stream object.

The current `thread_local ThreadSafeIOStream threadSafeCout` gives each thread
its own buffer and prefix, but a mutex stored as a normal member also gives
each thread a different mutex. That does **not** synchronize output between
threads.

Use:

```cpp
class ThreadSafeIOStream {
private:
    static std::mutex _outputMutex;
    std::string _prefix;
    std::ostringstream _buffer;
};
```

Define it once in `src/thread_safe_iostream.cpp`:

```cpp
std::mutex ThreadSafeIOStream::_outputMutex;
thread_local ThreadSafeIOStream threadSafeCout;
```

The templated `operator<<` can append to the current thread's private buffer.
The manipulator overload must lock the shared mutex and emit one complete
buffer:

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

Also protect `setPrefix()` and input operations with the same shared mutex.

---

## 10. WorkerPool

The PDF requires an `IJobs` interface and a worker pool with `addJob()`.

Fix the interface destructor:

```cpp
class IJobs {
public:
    virtual ~IJobs() = default;
    virtual void addJob(const std::function<void()>& job) = 0;
};
```

The destructor must be virtual because `WorkerPool` is used through an
interface.

In the worker loop:

- wait on the condition variable while the queue is empty and the pool is not
  stopping;
- when stopping, decide whether queued jobs should be drained or discarded;
- join every worker in the destructor;
- do not hold a mutex while executing user code.

The current worker loop holds `_queueMutex` while calling `job()`. Change the
scope so the job is popped under the lock, then executed after the lock is
released. Holding the queue mutex while user code runs blocks producers and
other workers.

Use one synchronization strategy consistently. The queue has its own mutex,
while the worker pool has `_queueMutex`; avoid checking the queue under one
lock and modifying it under another without a deliberate design.

---

## 11. PersistentWorker

The PDF requires named tasks that repeatedly execute until removed.

### Current critical bug

`addTask()` currently captures function parameters by reference:

```cpp
std::thread([&]() { ... name ... jobToExecute ... });
```

`name` and `jobToExecute` are parameters of `addTask()`. The thread can outlive
the function, so those references can dangle.

Capture them by value:

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

### Other required corrections

- Lock access to `_tasks` and `_toRemove` consistently.
- Reject duplicate task names instead of silently failing `insert()`.
- `removeTask()` must detect an unknown name; do not use `_tasks[name]`, since
  that creates a new entry.
- Ensure the removal flag exists before the thread starts.
- Do not hold `_guard` while executing user code.
- Catching exceptions from user tasks is optional, but an uncaught exception
  escaping a thread calls `std::terminate`; decide and document the policy.

---

## 12. Message

The PDF requires:

- `Message(int type)`;
- templated serialization operators;
- `type()`;
- detection of the message type by clients and servers.

Keep `type()` const:

```cpp
Type type() const;
```

and define it as:

```cpp
Message::Type Message::type() const {
    return _type;
}
```

The `operator>>` declaration should return `Message&`, not `const Message&`,
because deserialization consumes data from the message buffer:

```cpp
template<typename TType>
friend Message& operator>>(Message& message, TType& value) {
    message._data >> value;
    return message;
}
```

The serialized representation must contain the type and the payload. Check
that `deserialize()` rejects data shorter than the type field.

---

## 13. Client and Server API

The PDF expects client actions to receive a read-only message:

```cpp
std::function<void(const Message&)>
```

The server action should receive a client ID and a read-only message:

```cpp
std::function<void(long long&, const Message&)>
```

Keep the declarations, definitions, and `update()` loops consistent. When
calling a server callback that expects `long long&`, do not use a const
structured binding:

```cpp
for (auto& [clientID, message] : messages) {
    auto it = actions.find(message.type());
    if (it != actions.end() && it->second)
        it->second(clientID, message);
}
```

The client loop can use const messages:

```cpp
for (const auto& message : messages) {
    auto it = actions.find(message.type());
    if (it != actions.end() && it->second)
        it->second(message);
}
```

---

## 14. Network I/O correctness

The current network code compiles, but these are important runtime issues.

### 14.1 Do not assume one send transmits everything

TCP `send()` may write fewer bytes than requested. Add a helper that loops:

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

Use it for both the size header and the serialized message.

### 14.2 Read the size header through bytes

Do not use:

```cpp
&clientBuf.size + clientBuf.totalBytes
```

That performs pointer arithmetic in units of `sizeof(size_t)`, not bytes.
Use:

```cpp
char* sizeBytes =
    reinterpret_cast<char*>(&clientBuf.size) + clientBuf.totalBytes;
```

Then pass `sizeBytes` to `recv()`. Apply the same correction to the server.

### 14.3 Handle zero-length payloads

If a serialized message can have a zero-length payload, do not call `recv()`
with a zero length and then interpret the result as a disconnect. Transition
directly to message processing when the declared payload size is zero.

### 14.4 Wake receiver threads during shutdown

Both client and server receiver threads can block in `select()`. Setting an
atomic flag alone does not wake `select()`.

Before joining:

1. call `shutdown(socket, SHUT_RDWR)` where appropriate;
2. close the socket or use a dedicated wake-up mechanism;
3. set the running flag to false;
4. join the receiver;
5. clear the socket descriptor.

The exact order must avoid another thread using a closed descriptor. Keep all
socket state transitions synchronized.

### 14.5 Do not silently discard failed sends

The server currently catches send exceptions and continues while removing the
message from the queue. This loses messages without reporting the error.

At minimum, keep the message queued when a partial/failed send is retryable.
For a simple implementation, report the error and disconnect the affected
client rather than silently pretending the send succeeded.

---

## 15. IVector2 and IVector3

The PDF requires templated vector structs with arithmetic, comparisons,
length, normalization, dot product, and cross product.

### Current issues

- Methods take non-const references.
- Methods are not consistently marked `const`.
- Definitions are in `.cpp` files, so only explicitly instantiated `int` and
  `float` types work. Other template types will fail to link.
- Division by a zero component is not checked.

### Recommended declaration pattern

For `IVector2`:

```cpp
template<typename TType>
struct IVector2 {
    TType x;
    TType y;

    IVector2(TType x = 0, TType y = 0);

    float length() const;
    IVector2<float> normalize() const;
    float dot(const IVector2& other) const;
    IVector2 cross() const;

    IVector2 operator+(const IVector2& other) const;
    IVector2 operator-(const IVector2& other) const;
    IVector2 operator*(const IVector2& other) const;
    IVector2 operator/(const IVector2& other) const;
    bool operator==(const IVector2& other) const;
    bool operator!=(const IVector2& other) const;
};
```

Apply the corresponding pattern to `IVector3`, including `cross(const
IVector3& other) const`.

Fix `normalize()` for zero length. Returning a zero vector is one reasonable
policy:

```cpp
if (len == 0.0f)
    return IVector2<float>(0.0f, 0.0f);
```

For `operator!=`, use:

```cpp
return !(*this == other);
```

### Template placement

The normal solution is to put template declarations and definitions in the
header. If you keep definitions in `.cpp`, you are intentionally supporting
only the explicit instantiations at the bottom:

```cpp
template struct IVector2<int>;
template struct IVector2<float>;
```

That is less general than the templated API promised by the subject.

---

## 16. Random2DCoordinateGenerator

The PDF requires that the same seed and coordinates always produce the same
result, and that changing either changes the result.

The current use of a local PRNG seeded from `{seed, x, y}` satisfies the
determinism requirement. Keep the seed independent from the generator's
mutable random state.

Add `const` where appropriate:

```cpp
long long seed() const;
long long operator()(const long long& x,
                     const long long& y) const;
```

If you change the declarations, update the definitions too.

---

## 17. PerlinNoise2D

The mandatory API requires a `sample(x, y)` function. The current class also
provides `operator()`, which is fine.

Check:

- the permutation table is initialized before `sample()`;
- indices are masked safely for negative coordinates;
- the result is deterministic for one `PerlinNoise2D` instance;
- no forbidden allocation functions are used.

The current constructor uses `std::rand()` and the current time, so separate
instances are not deterministic relative to each other. If deterministic
reproducibility is desired, add a seed constructor or use a local
`std::mt19937` with an explicit seed. This is an improvement, not a separate
PDF method requirement.

---

## 18. Header and Makefile checks

Every header must compile independently. Test each header with:

```bash
for header in inc/*.hpp; do
    printf '#include "%s"\nint main() {}\n' "$(basename "$header")" \
        | c++ -x c++ -std=c++17 -Wall -Wextra -Werror -pedantic \
            -I inc -fsyntax-only -
done
```

The Makefile currently builds the library, but this line is misspelled and
unused:

```make
SRC = $(addpredix $(SRC_DIR), $(SRCS))
```

Either remove `SRC` because the pattern rule already uses `SRCS`, or correct
it to:

```make
SRC = $(addprefix $(SRC_DIR), $(SRCS))
```

Keep:

```make
CXXFLAGS = -Wall -Wextra -Werror -pedantic -std=c++17
```

C++17 is allowed because the subject requires C++11 or later.

---

## 19. Validation procedure

### Build the archive

```bash
make fclean
make
```

Expected result:

```text
libftpp.a
```

### Instantiate template APIs

Create a temporary test outside the repository:

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

Compile it against the archive:

```bash
c++ -std=c++17 -Wall -Wextra -Werror -pedantic \
    -I inc mandatory_smoke.cpp libftpp.a -pthread -o mandatory_smoke
./mandatory_smoke
```

This catches template errors that `make` alone does not catch.

### Pool behavior

Test all of these:

1. `resize(2)` followed by two successful acquisitions.
2. A third acquisition throws.
3. Releasing an object makes its slot available again.
4. Shrinking removes only available slots.
5. Shrinking below the number of acquired objects throws without invalidating
   them.
6. A type without a default constructor can be acquired with constructor
   arguments.
7. A constructor that throws returns the slot to the pool.

### Thread and task behavior

Test:

- starting a `Thread` twice;
- stopping a `Thread` before starting it;
- adding/removing a persistent task;
- destroying `PersistentWorker` while tasks are running;
- destroying `WorkerPool` while jobs are queued.

### Network behavior

Test:

- multiple messages in one connection;
- messages larger than one `send()`/`recv()` operation;
- client/server shutdown while idle;
- disconnect during an active receive;
- sending to an unknown client;
- failed sends.

---

## Recommended order

1. Fix Observer so the umbrella header can instantiate it.
2. Fix StateMachine callback storage.
3. Fix PersistentWorker captures and task-map locking.
4. Fix ThreadSafeIOStream's shared mutex.
5. Fix WorkerPool locking and virtual destruction.
6. Fix network byte offsets, full send/receive loops, and shutdown.
7. Fix vector const-correctness and template placement.
8. Review DataBuffer failure behavior.
9. Run the independent-header and smoke tests.

The Pool implementation can remain with aligned storage and shrinking; it is
not necessary to remove the shrink feature.
