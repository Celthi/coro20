#include <iostream>
#include <coroutine>
template <typename T>
class Sync
{
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle;
    struct promise_type
    {
        T value;
        Sync get_return_object()
        {
            return Sync{handle_type::from_promise(*this)};
        }
        std::suspend_always initial_suspend()
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            return {};
        }
        void return_void() {}
        void unhandled_exception() {}
        std::suspend_always yield_value(T v)
        {
            value = v;
            return {};
        }
    };
    Sync(handle_type h) : handle(h) {}
    Sync(Sync &&s) : handle(s.handle)
    {
        s.handle = nullptr;
    }
    ~Sync()
    {
        if (handle)
        {
            handle.destroy();
        }
    }
    void next()
    {
        if (handle.done())
        {
            return;
        }
        handle.resume();
    }
    T value()
    {
        return handle.promise().value;
    }
    // range access method
    struct Iter
    {
        Sync &sync;
        bool operator!=(Iter const &) const
        {
            return sync.handle && !sync.handle.done();
        }
        void operator++() { sync.next(); }

        T operator*() const
        {
            return sync.value();
        }
    };

    Iter begin()
    {
        next();
        return Iter{*this};
    }
    Iter end()
    {
        return Iter{*this};
    }
};

Sync<int> getNumber(int i)
{
    co_yield 42;
}
int main()
{
    for (auto i : getNumber(5))
    {
        std::cout << i << std::endl;
    }
    std::cout << "Hello, World!" << std::endl;
}
