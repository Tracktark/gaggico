#pragma once
#include "hardware/timer.h"
#include "inttypes.hpp"
#include "panic.hpp"
#include <pico/time.h>
#include <coroutine>

struct Awaiter;

struct Protocol {
    static inline char buffer[256];
    static inline bool has_coroutine = false;
    static inline Awaiter* curr_awaiter = nullptr;
    struct Promise {
        Protocol get_return_object() noexcept { return {}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() noexcept {}
        void return_void() noexcept {}

        void* operator new(usize len) {
            if (len > sizeof(buffer)) {
                panic(Error::COROUTINE_TOO_BIG);
            }
            has_coroutine = true;
            return buffer;
        }
        void operator delete(void*) {
            has_coroutine = false;
            curr_awaiter = nullptr;
        }
    };

    using promise_type = Promise;
    using Handle = std::coroutine_handle<Promise>;

    static Handle handle() { return Handle::from_address(buffer); }
};

struct Awaiter {
    virtual bool should_resume() = 0;
    void await_suspend(Protocol::Handle) {
        Protocol::curr_awaiter = this;
    }
    void await_resume() {
        Protocol::curr_awaiter = nullptr;
    }
};

struct next_cycle_t {
    bool await_ready() const { return false; }
    void await_suspend(Protocol::Handle) const {}
    void await_resume() const {}
};
constexpr next_cycle_t next_cycle {};

struct delay_ms : Awaiter {
    absolute_time_t resume_time;
    explicit delay_ms(u32 ms): resume_time(make_timeout_time_ms(ms)) {}

    bool should_resume() { return time_reached(resume_time); }
    bool await_ready() { return should_resume(); }
};

struct predicate : Awaiter {
    bool(*pred)();

    explicit predicate(bool(*pred)()): pred(pred) {}

    bool should_resume() { return pred(); }
    bool await_ready() { return pred(); }
};
