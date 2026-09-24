#ifndef POOL_HPP
#define POOL_HPP

#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

template<typename TType>
class Pool {
private:
    struct Slot {
        alignas(TType) unsigned char storage[sizeof(TType)];

        TType* object() {
            return std::launder(reinterpret_cast<TType*>(storage));
        }

        const TType* object() const {
            return std::launder(reinterpret_cast<const TType*>(storage));
        }
    };

public:
    class Object {
        friend class Pool;

    private:
        Object(Pool* pool, Slot* slot)
            : _pool(pool), _slot(slot) {}

    public:
        Object(Object&& other) noexcept
            : _pool(other._pool), _slot(other._slot) {
            other._pool = nullptr;
            other._slot = nullptr;
        }

        Object& operator=(Object&& other) noexcept {
            if (this == &other)
                return *this;

            release();
            _pool = other._pool;
            _slot = other._slot;
            other._pool = nullptr;
            other._slot = nullptr;
            return *this;
        }

        Object(const Object&) = delete;
        Object& operator=(const Object&) = delete;

        ~Object() {
            release();
        }

        TType* operator->() {
            return _slot->object();
        }

        const TType* operator->() const {
            return _slot->object();
        }

        TType& operator*() {
            return *_slot->object();
        }

        const TType& operator*() const {
            return *_slot->object();
        }

    private:
        void release() noexcept {
            if (!_pool || !_slot)
                return;

            _slot->object()->~TType();
            _pool->_available.push_back(_slot);
            _pool = nullptr;
            _slot = nullptr;
        }

        Pool* _pool;
        Slot* _slot;
    };

    void resize(const std::size_t& numberOfObjectsStored) {
        if (numberOfObjectsStored > _slots.size()) {
            const std::size_t numberToAdd =
                numberOfObjectsStored - _slots.size();

            for (std::size_t i = 0; i < numberToAdd; ++i) {
                std::unique_ptr<Slot> slot(new Slot);
                _available.push_back(slot.get());
                _slots.push_back(std::move(slot));
            }
            return;
        }

        const std::size_t numberToRemove =
            _slots.size() - numberOfObjectsStored;

        if (numberToRemove > _available.size())
            throw NoAvailableObjToRemoveException();

        for (std::size_t i = 0; i < numberToRemove; ++i) {
            Slot* slot = _available.back();
            _available.pop_back();

            for (auto it = _slots.begin(); it != _slots.end(); ++it) {
                if (it->get() == slot) {
                    _slots.erase(it);
                    break;
                }
            }
        }
    }

    template<typename... TArgs>
    Object acquire(TArgs&&... args) {
        if (_available.empty())
            throw NoAvailableObjectsException();

        Slot* slot = _available.back();
        _available.pop_back();

        try {
            new (slot->storage) TType(std::forward<TArgs>(args)...);
        } catch (...) {
            _available.push_back(slot);
            throw;
        }

        return Object(this, slot);
    }

    class NoAvailableObjectsException : public std::runtime_error {
    public:
        NoAvailableObjectsException()
            : std::runtime_error("Pool: no available objects.") {}
    };

    class NoAvailableObjToRemoveException : public std::runtime_error {
    public:
        NoAvailableObjToRemoveException()
            : std::runtime_error("Pool: no available objects to remove.") {}
    };

private:
    std::vector<std::unique_ptr<Slot>> _slots;
    std::vector<Slot*> _available;
};

#endif
