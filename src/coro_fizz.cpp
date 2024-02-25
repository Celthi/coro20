#include <iostream>
#include <coroutine>
#include <optional>
#include <source_location>

struct YieldAwaitable
{
    std::coroutine_handle<> consumer_coro_handle;
    YieldAwaitable() : consumer_coro_handle(nullptr) {}
    explicit YieldAwaitable(std::coroutine_handle<> h) : consumer_coro_handle(h) {}
    constexpr bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept
    {
        if (consumer_coro_handle)
        {
            return consumer_coro_handle;
        }
        return std::noop_coroutine();
    }
    constexpr void await_resume() const noexcept {}
};

using Value = int;
// The coroutine that generates numbers
class GenNumber
{
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle; // the coroutine handle
    // rule of zero
    GenNumber() = default;
    GenNumber(const GenNumber &) = delete;            // 1. no copy constructor
    GenNumber &operator=(const GenNumber &) = delete; // 2. no copy assignment
    explicit GenNumber(handle_type h) : handle(h) {}  // 3. constructor
    ~GenNumber()                                      // 4. destructor
    {
        if (handle)
        {
            handle.destroy();
        }
    }
    GenNumber(GenNumber &&s) : handle(s.handle) // 5. move constructor
    {
        s.handle = nullptr;
    }
    GenNumber &operator=(GenNumber &&s) // 6. move assignment
    {
        handle = s.handle;
        s.handle = nullptr;
        return *this;
    }

    class GenNumberAwaiter
    {
    public:
        handle_type producer_handler;
        explicit GenNumberAwaiter(handle_type p) : producer_handler(p) {}

        bool await_ready() const { return false; }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const
        {
            return producer_handler;  
        }

        std::optional<Value> await_resume()
        {
            return producer_handler.promise().value;
        }
    };
    // promise_type is the place to store data about the coroutine result
    // and the policy for the coroutine
    struct promise_type
    {
        int limit;
        std::optional<Value> value;
        std::coroutine_handle<> consumer_coro_handle;
        promise_type() = default;
        explicit promise_type(int limit) : limit(limit) {}
        GenNumber get_return_object()
        {
            return GenNumber{handle_type::from_promise(*this)};
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        // the coroutine will not return value
        void return_void() const {}
        void unhandled_exception() const {}
        // the co_yield expression will call this function
        YieldAwaitable yield_value(Value v)
        {
            value = v;
            return YieldAwaitable{consumer_coro_handle};
        }
    };
};

// the consumer coroutine
class Consumer
{
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle; // Consumer coroutine handle
    struct promise_type
    {
        std::optional<Value> value;                                     // the value to be returned
        std::coroutine_handle<GenNumber::promise_type> producer_handle; // the producer coroutine handle
        promise_type(const GenNumber &source, int divisor) : producer_handle(source.handle) {}
        promise_type(const promise_type &) = delete;
        promise_type &operator=(const promise_type &) = delete;
        Consumer get_return_object()
        {
            return Consumer{handle_type::from_promise(*this)};
        }
        std::suspend_always initial_suspend() const { return {}; }
        std::suspend_always final_suspend() const noexcept { return {}; }
        void return_void() const {}
        void unhandled_exception() const {}
        // await_transform method
        GenNumber::GenNumberAwaiter await_transform(const GenNumber &source)
        {

            auto awaitable = GenNumber::GenNumberAwaiter{source.handle};
            source.handle.promise().consumer_coro_handle = std::coroutine_handle<promise_type>::from_promise(*this);
            return awaitable;
        }
        std::suspend_always yield_value(std::optional<Value> v)
        {
            value = v;
            return {};
        }
    };

    explicit Consumer(handle_type h) : handle(h)
    {
    }
    Consumer(Consumer &&s) : handle(s.handle)
    {
        s.handle = nullptr;
    }
    Consumer &operator=(Consumer &&s) noexcept
    {
        handle = s.handle;
        s.handle = nullptr;
        return *this;
    }
    ~Consumer()
    {
        if (handle)
        {
            handle.destroy();
        }
    }
    std::optional<Value> next_value()
    {
        if (handle.done())
        {
            return {};
        }
        handle.promise().producer_handle.promise().value = {};
        handle.resume();
        auto v = handle.promise().producer_handle.promise().value;
        return v;
    }
    bool done()
    {
        return handle.done();
    }
};

GenNumber generate_numbers(int limit)
{

    for (int i = 1; i <= limit; i++)
    {
        Value v = i;
        co_yield v;
    }
}
Consumer consume_number(GenNumber source, int divisor)
{
    std::cout<<"consume_number"<<std::endl;
    while (std::optional<Value> vopt = co_await source) // await_transform -> GenNumber::GenNumberAwaiter -> GenNumber::await_suspend -> gen_number::resume() -> *yieldawaitable::await_suspend* -> consumer_coro_handle.resume() -> await_resume
    {
        if (*vopt % divisor == 0)
        {
            co_yield vopt;
        }
    }
}

int main()
{
    GenNumber c = generate_numbers(9);
    auto res = consume_number(std::move(c), 3);
    while (std::optional<Value> vopt = res.next_value())
    {
        std::cout << "value: " << *vopt << " " << std::endl;
    }
}
