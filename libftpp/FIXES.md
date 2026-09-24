# How to fix the mandatory part of libftpp

This is the checklist I would use to repair the mandatory implementations in `libftpp` so they match the assignment and behave safely.

## 1) Observer: do not store pointers to std::function

Problem:
- In `inc/observer.hpp`, the code stores `const std::function<void()>*` in a map.
- Those pointers point to temporary lambda objects and become invalid as soon as `subscribe()` returns.
- This can crash or silently misbehave.

Correct design:
- Store copies of the function objects in the container.
- `std::function` is copyable and safe to store by value.

Fix:

```cpp
#ifndef OBSERVER_HPP
#define OBSERVER_HPP

#include <functional>
#include <unordered_map>
#include <vector>

template<typename TEvent>
class Observer {
public:
    void subscribe(const TEvent& event, const std::function<void()>& lambda) {
        _events[event].push_back(lambda);
    }

    void notify(const TEvent& event) {
        auto it = _events.find(event);
        if (it == _events.end())
            return;

        for (auto& callback : it->second) {
            if (callback)
                callback();
        }
    }

private:
    std::unordered_map<TEvent, std::vector<std::function<void()>>> _events;
};

#endif
```

Why this is better:
- No dangling pointers.
- No lifetime issues.
- Easy to unsubscribe later if needed.

---

## 2) StateMachine: fix ownership, transitions, and missing checks

Problem:
- `StateMachine` stores raw `TState* _curstate` and deletes it manually.
- The map uses `std::vector<const std::function<void()>*>` again, so same dangling pointer issue.
- There is no clear validation when a state or transition has not been configured.

Better implementation:
- Store `std::function<void()>` by value.
- Keep current state in a value, not a pointer.
- Validate state existence before `update()`/`transitionTo()`.
- Use a proper transition key struct with both states.

Example:

```cpp
#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

template<typename TState>
class StateMachine {
public:
    struct TransitionKey {
        TState from;
        TState to;

        bool operator==(const TransitionKey& other) const {
            return from == other.from && to == other.to;
        }
    };

    struct TransitionKeyHash {
        std::size_t operator()(const TransitionKey& key) const {
            std::size_t h1 = std::hash<TState>{}(key.from);
            std::size_t h2 = std::hash<TState>{}(key.to);
            return h1 ^ (h2 << 1);
        }
    };

    void addState(const TState& state) {
        _stateActions[state];
        if (!_hasCurrentState)
            _currentState = state, _hasCurrentState = true;
    }

    void addAction(const TState& state, const std::function<void()>& lambda) {
        _stateActions[state].push_back(lambda);
    }

    void addTransition(const TState& startState,
                       const TState& finalState,
                       const std::function<void()>& lambda) {
        _transitions[{startState, finalState}].push_back(lambda);
    }

    void transitionTo(const TState& state) {
        if (!_hasCurrentState)
            throw std::runtime_error("StateMachine: no current state");

        auto it = _transitions.find({ _currentState, state });
        if (it == _transitions.end())
            throw std::invalid_argument("StateMachine: invalid transition");

        _currentState = state;
        for (const auto& callback : it->second) {
            if (callback)
                callback();
        }
    }

    void update() {
        if (!_hasCurrentState)
            throw std::runtime_error("StateMachine: current state not set");

        auto it = _stateActions.find(_currentState);
        if (it == _stateActions.end())
            throw std::runtime_error("StateMachine: state not found");

        for (const auto& callback : it->second) {
            if (callback)
                callback();
        }
    }

private:
    TState _currentState{};
    bool _hasCurrentState = false;
    std::unordered_map<TState, std::vector<std::function<void()>>> _stateActions;
    std::unordered_map<TransitionKey, std::vector<std::function<void()>>, TransitionKeyHash> _transitions;
};

#endif
```

Important:
- `std::hash<TState>` must exist for the type used as the state.
- If your state type is custom, implement a custom hash or use `std::string`/`int`-like states.

---

## 3) ThreadSafeQueue: protect every operation and `empty()`

Problem:
- `empty()` is not mutex-protected.
- This can race with pushes/pops.
- It is also a bad idea to expose mutability without locking.

Correct version:

```cpp
#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <deque>
#include <mutex>
#include <stdexcept>

template<typename TType>
class ThreadSafeQueue {
public:
    void push_back(const TType& value) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push_back(value);
    }

    void push_front(const TType& value) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push_front(value);
    }

    TType pop_back() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty())
            throw std::runtime_error("ThreadSafeQueue: empty queue");

        TType value = _queue.back();
        _queue.pop_back();
        return value;
    }

    TType pop_front() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty())
            throw std::runtime_error("ThreadSafeQueue: empty queue");

        TType value = _queue.front();
        _queue.pop_front();
        return value;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.empty();
    }

private:
    mutable std::mutex _mutex;
    std::deque<TType> _queue;
};

#endif
```

Note:
- `empty()` is now synchronized.
- All access is through the same mutex.

---

## 4) Thread: do not start or stop twice

Problem:
- `Thread::start()` blindly starts a thread, even if already started.
- `Thread::stop()` joins an unstarted thread.
- `std::thread` is not copyable, and a default-constructed thread is valid but not running.

Clean fix:

```cpp
#ifndef THREAD_HPP
#define THREAD_HPP

#include <functional>
#include <string>
#include <thread>
#include <stdexcept>

class Thread {
public:
    Thread(const std::string& name, std::function<void()> func)
        : _name(name), _func(std::move(func)) {}

    void start() {
        if (_thread.joinable())
            throw std::runtime_error("Thread: already started");
        if (!_func)
            throw std::runtime_error("Thread: no function assigned");

        _thread = std::thread(&Thread::run, this);
    }

    void stop() {
        if (!_thread.joinable())
            return;
        _thread.join();
    }

private:
    void run() {
        // optional: use threadSafeCout.setPrefix(_name);
        _func();
    }

private:
    std::string _name;
    std::function<void()> _func;
    std::thread _thread;
};

#endif
```

If you want the printed prefix behavior:

```cpp
void run() {
    threadSafeCout.setPrefix(_name);
    _func();
}
```

---

## 5) Pool: keep shrink, but fix object lifetime

The subject says:
- `Pool::Object` must provide access to a preallocated object.
- When the object is released, call its destructor but do not deallocate memory.

Do **not** remove shrinking. Shrinking is valid if the pool removes only available objects and throws when a live `Pool::Object` would be invalidated.

The real lifetime problem is using `std::unique_ptr<TType>` for objects that are also reconstructed with placement `new`. That constructs `TType` once during allocation, constructs another `TType` over the first one in `acquire()`, and can later destroy the same storage twice. Use untyped aligned storage for the slots instead.

This version supports both growth and shrinking:

```cpp
#ifndef POOL_HPP
#define POOL_HPP

#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

template<typename TType>
class Pool {
private:
    struct Slot {
        typename std::aligned_storage<sizeof(TType), alignof(TType)>::type storage;
        bool constructed = false;

        TType* object() {
            return reinterpret_cast<TType*>(&storage);
        }
    };

public:
    class Object {
        friend class Pool;

        Object(Pool* pool, Slot* slot)
            : _pool(pool), _slot(slot) {}

    public:
        Object(Object&& other) noexcept
            : _pool(other._pool), _slot(other._slot) {
            other._pool = nullptr;
            other._slot = nullptr;
        }

        Object(const Object&) = delete;
        Object& operator=(const Object&) = delete;

        ~Object() {
            if (_pool && _slot) {
                _slot->object()->~TType();
                _slot->constructed = false;
                _pool->_available.push_back(_slot);
            }
        }

        TType* operator->() {
            return _slot->object();
        }

        const TType* operator->() const {
            return _slot->object();
        }

    private:
        Pool* _pool;
        Slot* _slot;
    };

    void resize(std::size_t count) {
        while (_slots.size() < count) {
            auto slot = std::make_unique<Slot>();
            _available.push_back(slot.get());
            _slots.push_back(std::move(slot));
        }

        while (_slots.size() > count) {
            if (_available.empty())
                throw std::runtime_error(
                    "Pool: cannot shrink while all remaining objects are acquired");

            Slot* removable = _available.back();
            _available.pop_back();

            for (auto it = _slots.begin(); it != _slots.end(); ++it) {
                if (it->get() == removable) {
                    _slots.erase(it);
                    break;
                }
            }
        }
    }

    template<typename... TArgs>
    Object acquire(TArgs&&... args) {
        if (_available.empty())
            throw std::runtime_error("Pool: no available object");

        Slot* slot = _available.back();
        _available.pop_back();
        new (&slot->storage) TType(std::forward<TArgs>(args)...);
        slot->constructed = true;
        return Object(this, slot);
    }

private:
    std::vector<std::unique_ptr<Slot>> _slots;
    std::vector<Slot*> _available;
};

#endif
```

Notes:
- The pool must outlive every `Pool::Object` acquired from it.
- `Object` must be move-only; otherwise two wrappers could destroy and release one slot twice.
- Growth adds unconstructed storage. `acquire()` is the only place that constructs `TType`.
- Shrinking removes only available slots. It must never invalidate an acquired object.

---

## 6) Memento: store a real snapshot and keep the contract clear

Problem:
- The current `Memento` is only a wrapper around `DataBuffer`, but the actual logic is vague.
- It should be easy to save and restore state using a snapshot container.

Recommended design:
- In the class that should save itself, define `_saveToSnapshot` and `_loadFromSnapshot`.
- Use `DataBuffer` serialization for the data you want to preserve.

Example:

```cpp
#ifndef MEMENTO_HPP
#define MEMENTO_HPP

#include "data_buffer.hpp"

class Memento {
public:
    using Snapshot = DataBuffer;

    virtual ~Memento() = default;

    Snapshot save() const {
        Snapshot snapshot;
        _saveToSnapshot(snapshot);
        return snapshot;
    }

    void load(const Snapshot& snapshot) {
        Snapshot copy = snapshot;
        _loadFromSnapshot(copy);
    }

private:
    virtual void _saveToSnapshot(Snapshot& snapshot) const = 0;
    virtual void _loadFromSnapshot(Snapshot& snapshot) = 0;
};

#endif
```

Example class using it:

```cpp
class Player : public Memento {
private:
    int _hp = 100;
    int _x = 0;
    int _y = 0;

    void _saveToSnapshot(Snapshot& snapshot) const override {
        snapshot << _hp << _x << _y;
    }

    void _loadFromSnapshot(Snapshot& snapshot) override {
        snapshot >> _hp >> _x >> _y;
    }
};
```

Why this is correct:
- The snapshot is explicit.
- Save/load is easy to reason about.
- The object decides what is restored.

---

## 7) Client and Server networking: align with the PDF contract

The assignment expects callback signatures like:

```cpp
void defineAction(const Message::Type& messageType,
                  const std::function<void(const Message& msg)>& action);
```

Your implementation uses non-const references, which is inconsistent with the required API, and the behavior is harder to reason about.

Fix the function signatures to match the subject:

```cpp
using Action = std::function<void(const Message&)>;

void defineAction(const Message::Type& messageType, const Action& action);
```

Then update the logic:

```cpp
void Client::update() {
    if (!_isConnected)
        throw NoConnectException();

    std::vector<Message> messages;
    std::unordered_map<Message::Type, Action> actions;

    {
        std::lock_guard<std::mutex> lock(_mtx);
        messages = std::move(_msgs);
        actions = _actions;
    }

    for (const auto& msg : messages) {
        auto it = actions.find(msg.type());
        if (it != actions.end() && it->second)
            it->second(msg);
    }
}
```

The same logic applies on the server side:

```cpp
using Action = std::function<void(long long, const Message&)>;
```

And in `update()`:

```cpp
for (const auto& [clientID, msg] : procList) {
    auto it = actions.find(msg.type());
    if (it != actions.end() && it->second)
        it->second(clientID, msg);
}
```

This keeps the semantics aligned with the PDF and avoids turning `Message` into a mutable object when it should be read-only.

---

## 8) IVector2 / IVector3: fix operators, normalization, and const correctness

Your vector code mostly compiles, but there are several important correctness issues.

### Problems
- `operator!=` uses `&&` instead of `||`.
- Methods take non-const references when they should be const references.
- `normalize()` can divide by zero.
- `dot()` and `cross()` should be const-correct.

### Correct version for `IVector2`

```cpp
#ifndef IVECTOR2_HPP
#define IVECTOR2_HPP

#include <cmath>

template<typename TType>
struct IVector2 {
    TType x;
    TType y;

    IVector2(TType x = 0, TType y = 0) : x(x), y(y) {}

    float length() const {
        return std::sqrt(static_cast<float>(x * x + y * y));
    }

    IVector2<float> normalize() const {
        float len = length();
        if (len == 0.0f)
            return IVector2<float>(0.0f, 0.0f);
        return IVector2<float>(static_cast<float>(x) / len,
                               static_cast<float>(y) / len);
    }

    float dot(const IVector2& other) const {
        return static_cast<float>(x * other.x + y * other.y);
    }

    IVector2 cross() const {
        return IVector2(-y, x);
    }

    IVector2 operator+(const IVector2& other) const {
        return IVector2(x + other.x, y + other.y);
    }

    IVector2 operator-(const IVector2& other) const {
        return IVector2(x - other.x, y - other.y);
    }

    IVector2 operator*(const IVector2& other) const {
        return IVector2(x * other.x, y * other.y);
    }

    IVector2 operator/(const IVector2& other) const {
        return IVector2(x / other.x, y / other.y);
    }

    bool operator==(const IVector2& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const IVector2& other) const {
        return !(*this == other);
    }
};

#endif
```

And `IVector3` follows the same pattern.

---

## 9) ThreadSafeIOStream: fix the prefix logic to be thread-local and safe

Your `ThreadSafeIOStream` class is near-correct but the current implementation does not guarantee correct output order under concurrency.

The simplest good fix:
- Keep a mutex around the whole output operation.
- Keep a per-thread prefix value.
- Print the buffer in one atomic chunk.

Example:

```cpp
#ifndef THREAD_SAFE_IOSTREAM_HPP
#define THREAD_SAFE_IOSTREAM_HPP

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

class ThreadSafeIOStream {
public:
    void setPrefix(const std::string& prefix) {
        std::lock_guard<std::mutex> lock(_mutex);
        _prefix = prefix;
    }

    template<typename T>
    ThreadSafeIOStream& operator<<(const T& value) {
        std::lock_guard<std::mutex> lock(_mutex);
        _buffer << value;
        return *this;
    }

    ThreadSafeIOStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::cout << _prefix << _buffer.str();
        manip(std::cout);
        _buffer.str("");
        _buffer.clear();
        return *this;
    }

    template<typename T>
    void prompt(const std::string& question, T& dest) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::cout << _prefix << question;
        std::cin >> dest;
    }

private:
    std::mutex _mutex;
    std::string _prefix;
    std::ostringstream _buffer;
};

extern thread_local ThreadSafeIOStream threadSafeCout;

#endif
```

This makes the prefix and output line operations coherent.

---

## 10) Quick repair list (do these first)

If you want the fastest path to a working mandatory submission, fix these in order:

1. Observer
2. StateMachine
3. ThreadSafeQueue
4. Thread
5. Vector operators / normalization
6. Client / Server callback API
7. Memento contract
8. ThreadSafeIOStream prefix logic
9. Pool lifetime rules

---

## 11) How to validate after fixing

Run:

```bash
make clean
make
```

Then compile a tiny test to check the main behaviors:

```cpp
#include "libftpp.hpp"

int main() {
    Observer<int> observer;
    observer.subscribe(42, [](){ std::cout << "event 42\n"; });
    observer.notify(42);

    StateMachine<int> machine;
    machine.addState(0);
    machine.addState(1);
    machine.addTransition(0, 1, [](){ std::cout << "transition\n"; });
    machine.transitionTo(1);

    ThreadSafeQueue<int> q;
    q.push_back(3);
    std::cout << q.pop_front() << "\n";

    IVector2<int> v(3, 4);
    std::cout << v.length() << "\n";
}
```

If it compiles and prints sensible output, you are very close to a correct mandatory submission.

---

## Final note

The project is not “totally broken”, but several mandatory pieces are not safe or spec-compliant. The biggest issues are the lifetime of callback storage and the state/transition logic.

Fixing those first will make the library behave much closer to the assignment requirements.
