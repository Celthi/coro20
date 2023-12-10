#include <coroutine>
#include <string>
#include <iostream>

template <typename T>
class UserFacing
{
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle;
    class YieldAwaiter
    {
    public:
        promise_type *consumer;
        explicit YieldAwaiter(promise_type *h) : consumer(h) {}
        bool await_ready()
        {
            return false;
        }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h)
        {
            if (consumer) 
            {
                return handle_type::from_promise(*consumer);
            }
            return std::noop_coroutine();
        }
        void await_resume() {}
    };

    class DataProducerAwaiter
    {
    public:
        promise_type *producer;
        explicit DataProducerAwaiter(promise_type *p) : producer(p) {}
        bool await_ready() { return false; }

        std::coroutine_handle<> await_suspend(handle_type h)
        {
            producer->value = std::nullopt;
            producer->consumer = &h.promise();
            return handle_type::from_promise(*producer);
        }
        std::optional<T> await_resume() { return producer->value; }
    };
    DataProducerAwaiter operator co_await()
    {
        return DataProducerAwaiter{&this->handle.promise()};
    }
    struct promise_type
    {
        promise_type *consumer;
        std::optional<T> value;
        UserFacing<T> get_return_object()
        {
            return UserFacing{handle_type::from_promise(*this)};
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
        auto yield_value(T v)
        {
            value = v;

            return YieldAwaiter{consumer};
        }
    };

    UserFacing(handle_type h) : handle(h) {}
    UserFacing(UserFacing &&s) : handle(s.handle)
    {
        s.handle = nullptr;
    }
    UserFacing &operator=(UserFacing &&s)
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
    std::optional<T> next_value()
    {
        handle.promise().value = std::nullopt;
        resume();
        return handle.promise().value;
    }

    void resume()
    {
        if (!handle.done())
            handle.resume();
    }
};

class Value
{
public:
    int number;
    std::string text;
};

UserFacing<Value> generate_number(int limit)
{
    for (int i = 0; i < limit; i++)
    {
        Value v;
        v.number = i;
        co_yield v;
    }
}

UserFacing<Value> check_multiple(UserFacing<Value> gen, int divisor, std::string text)
{
    while (std::optional<Value> v = co_await gen)
    {
        auto val = *v;
        if (val.number % divisor == 0)
        {
            val.text.append(text);
        }
        co_yield val;
    }
}

int main()
{

    auto c = generate_number(20);
    c = check_multiple(std::move(c), 3, "Fizz");
    c = check_multiple(std::move(c), 5, "Buzz");
    while (auto v = c.next_value())
    {
        std::cout << v->number << " " << v->text << std::endl;
    }
}
