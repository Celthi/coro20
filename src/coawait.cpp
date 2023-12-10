#include <coroutine>
#include <iostream>
size_t level = 0;
std::string INDENT = "-";

class Trace
{
public:
    Trace()
    {
        in_level();
    }
    ~Trace()
    {
        level -= 1;
    }
    void in_level()
    {
        level += 1;
        std::string res(INDENT);
        for (size_t i = 0; i < level; i++)
        {
            res.append(INDENT);
        };
        std::cout << res;
    }
};

template <typename T>
struct sync
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type coro;

    sync(handle_type h)
        : coro(h)
    {
        Trace t;
        std::cout << "Sync: Created a sync object" << std::endl;
    }
    sync(const sync &) = delete;
    sync(sync &&s)
        : coro(s.coro)
    {
        Trace t;
        std::cout << "Sync: Sync moved leaving behind a husk" << std::endl;
        s.coro = nullptr;
    }
    ~sync()
    {
        Trace t;
        std::cout << "Sync: Sync gone" << std::endl;
        if (coro)
            coro.destroy();
    }
    sync &operator=(const sync &) = delete;
    sync &operator=(sync &&s)
    {
        coro = s.coro;
        s.coro = nullptr;
        return *this;
    }

    T get()
    {
        Trace t;
        std::cout << "Sync: We got asked for the return value..." << std::endl;
        return coro.promise().value;
    }
    struct promise_type
    {
        T value;
        promise_type()
        {
            Trace t;
            std::cout << "Sync-Promise: Promise created" << std::endl;
        }
        ~promise_type()
        {
            Trace t;
            std::cout << "Sync-Promise: Promise died" << std::endl;
        }

        auto get_return_object()
        {
            Trace t;
            std::cout << "Sync-Promise: Send back a sync" << std::endl;
            return sync<T>{handle_type::from_promise(*this)};
        }
        auto initial_suspend()
        {
            Trace t;
            std::cout << "Sync-Promise: Started the coroutine, don't stop now!" << std::endl;
            return std::suspend_never{};
        }
        auto return_value(T v)
        {
            Trace t;
            std::cout << "Sync-Promise: Got an answer of " << v << std::endl;
            value = v;
            return std::suspend_never{};
        }
        auto final_suspend() noexcept
        {
            Trace t;
            std::cout << "Sync-Promise: Finished the coro" << std::endl;
            return std::suspend_always{};
        }
        void unhandled_exception()
        {
            std::exit(1);
        }
    };
    void resume() {
        Trace t;
        std::cout << "Sync: About to resume the sync" << std::endl;
        coro.resume();
    }
};

template <typename T>
struct lazy
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type coro;

    lazy(handle_type h)
        : coro(h)
    {
        Trace t;
        std::cout << "Lazy: Created a lazy object" << std::endl;
    }
    lazy(const lazy &) = delete;
    lazy(lazy &&s)
        : coro(s.coro)
    {
        Trace t;
        std::cout << "Lazy: lazy moved leaving behind a husk" << std::endl;
        s.coro = nullptr;
    }
    ~lazy()
    {
        Trace t;
        std::cout << "Lazy: lazy gone" << std::endl;
        if (coro)
            coro.destroy();
    }
    lazy &operator=(const lazy &) = delete;
    lazy &operator=(lazy &&s)
    {
        coro = s.coro;
        s.coro = nullptr;
        return *this;
    }

    T get()
    {
        Trace t;
        std::cout << "Lazy: We got asked for the return value..." << std::endl;
        return coro.promise().value;
    }
    struct promise_type
    {
        T value;
        promise_type()
        {
            Trace t;
            std::cout << "Lazy-Promise: Promise created" << std::endl;
        }
        ~promise_type()
        {
            Trace t;
            std::cout << "Lazy-Promise: Promise died" << std::endl;
        }

        auto get_return_object()
        {
            Trace t;
            std::cout << "Lazy-Promise: Send back a lazy" << std::endl;
            return lazy<T>{handle_type::from_promise(*this)};
        }
        auto initial_suspend()
        {
            Trace t;
            std::cout << "Lazy-Promise: Started the coroutine, put the brakes on!" << std::endl;
            return std::suspend_always{};
        }
        auto return_value(T v)
        {
            Trace t;
            std::cout << "Lazy-Promise: Got an answer of " << v << std::endl;
            value = v;
            return std::suspend_never{};
        }
        auto final_suspend() noexcept
        {
            Trace t;
            std::cout << "Lazy-Promise: Finished the coro" << std::endl;
            return std::suspend_always{};
        }
        void unhandled_exception()
        {
            std::exit(1);
        }
    };
    bool await_ready()
    {
        const auto ready = this->coro.done();
        Trace t;
        std::cout << "Lazy: Await " << (ready ? "is ready" : "isn't ready") << std::endl;
        return this->coro.done();
    }
    handle_type await_suspend(std::coroutine_handle<> awaiting)
    {
        // {
        //     Trace t;
        //     std::cout << "Lazy: About to resume the lazy" << std::endl;
        //     this->coro.resume();
        // }
        // Trace t;
        // std::cout << "Lazy: About to resume the awaiter" << std::endl;
        // awaiting.resume();
        return this->coro;
    }
    auto await_resume()
    {
        const auto r = this->coro.promise().value;
        Trace t;
        std::cout << "Lazy: Await value is returned: " << r << std::endl;
        return r;
    }
};
lazy<std::string> read_data()
{
    Trace t;
    std::cout << "reading_data(): Reading data..." << std::endl;
    co_return "reading_data: billion$!";
}

sync<int> reply()
{
    std::cout << "reply(): Started await_answer" << std::endl;
    auto a = co_await read_data();
    std::cout << "reply(): read result is " << a << std::endl;
    co_return 42;
}

int main()
{
    std::cout << "main: Start main()\n";
    auto a = reply();
    a.resume();
    return a.get();
}
