#include <iostream>
#include <coroutine>
#include <optional>
#include <source_location>


struct YieldAwaitable {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
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
        handle_type await_suspend(std::coroutine_handle<>) const
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

        explicit promise_type(int limit) : limit(limit) { }
        GenNumber get_return_object()
        {
            return GenNumber{handle_type::from_promise(*this)};
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        // the coroutine will not return value
        void return_void() const { }
        void unhandled_exception() const { }
        // the co_yield expression will call this function
        YieldAwaitable yield_value(Value v) {
            value = v;
            return {};
        }
    };
};

// the consumer coroutine
class UserFacing
{
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle; // UserFacing coroutine handle
    struct promise_type
    {
        std::optional<Value> value;                                     // the value to be returned
        std::coroutine_handle<GenNumber::promise_type> producer_handle; // the producer coroutine handle
        promise_type(const GenNumber &source, int divisor, std::string fizz) : producer_handle(source.handle) { }
        promise_type(const promise_type &) = delete;
        promise_type &operator=(const promise_type &) = delete;
        UserFacing get_return_object()
        {
            return UserFacing{handle_type::from_promise(*this)};
        }
        std::suspend_always initial_suspend() const { return {}; }
        std::suspend_always final_suspend() const noexcept { return {}; }
        void return_void() const { }
        void unhandled_exception() const { }
        // await_transform method
        GenNumber::GenNumberAwaiter await_transform(const GenNumber &source) const
        {
            return GenNumber::GenNumberAwaiter{source.handle};
        }
    };

    explicit UserFacing(handle_type h) : handle(h)
    {
    }
    UserFacing(UserFacing &&s) : handle(s.handle)
    {
        s.handle = nullptr;
    }
    UserFacing &operator=(UserFacing &&s) noexcept
    {
        handle = s.handle;
        s.handle = nullptr;
        return *this;
    }
    ~UserFacing()
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
UserFacing check_multiple(GenNumber source, int divisor, std::string fizz)
{
    while (std::optional<Value> vopt = co_await source)
    {
        if (*vopt % divisor == 0)
        {
        }
    }
    std::cout<<"done"<<std::endl;
}

int main()
{
    GenNumber c = generate_numbers(3);
    auto res = check_multiple(std::move(c), 3, "Fizz");
    while (std::optional<Value> vopt = res.next_value())
    {
        std::cout << "value: " << *vopt << " " << std::endl;
    }
}
