#include "hardware/sync.h"
#include "inttypes.hpp"
#include "pico/lock_core.h"

template <typename T, usize Capacity>
class Queue {
    lock_core lock;
    T data[Capacity];

    usize size_;
    usize front;
    usize back;

public:
    void init() {
        front = 0;
        back = 0;
        size_ = 0;
        lock_init(&lock, next_striped_spin_lock_num());
    }


    const T& peek() const {
        return this->data[front];
    }

    usize size() const {
        u32 save = spin_lock_blocking(lock.spin_lock);
        usize size = this->size_;
        spin_unlock(lock.spin_lock, save);
        return size;
    }

    void push_blocking(const T& data) {
        add_internal(data, true);
    }

    bool try_push(const T& data) {
        return add_internal(data, false);
    }

    void pop_blocking() {
        remove_internal(true);
    }

    bool try_pop() {
        return remove_internal(false);
    }

private:
    constexpr void inc_index(usize& index) const {
        index = (index + 1) % Capacity;
    }

    bool add_internal(const T& data, bool block) {
        while (true) {
            u32 save = spin_lock_blocking(lock.spin_lock);

            if (size_ < Capacity) {
                this->data[back] = data;
                size_++;
                inc_index(back);
                lock_internal_spin_unlock_with_notify(&lock, save);
                return true;
            }

            if (block) {
                lock_internal_spin_unlock_with_wait(&lock, save);
            } else {
                spin_unlock(lock.spin_lock, save);
                return false;
            }
        }
    }

    bool remove_internal(bool block) {
        while (true) {
            u32 save = spin_lock_blocking(lock.spin_lock);

            if (size_ > 0) {
                size_--;
                inc_index(front);
                lock_internal_spin_unlock_with_notify(&lock, save);
                return true;
            }

            if (block) {
                lock_internal_spin_unlock_with_wait(&lock, save);
            } else {
                spin_unlock(lock.spin_lock, save);
                return false;
            }
        }
    }
};
