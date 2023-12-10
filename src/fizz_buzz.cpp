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
    struct promise_type
    {

        T value;
        UserFacing get_return_object()
        {
            return UserFacing{handle_type::from_promise(*this)};
        }
        std::suspend_never initial_suspend()
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

    UserFacing(handle_type h) : handle(h) {}
    UserFacing(UserFacing &&s) : handle(s.handle)
    {
        s.handle = nullptr;
    }
    ~UserFacing()
    {
        if (handle)
        {
            handle.destroy();
        }
    }
    struct Iter {
        UserFacing &sync;
        bool operator!=(Iter const &) const { return !sync.handle.done(); }
        void operator++() { sync.handle.resume(); }
        T const &operator*() const { return sync.handle.promise().value; }
    };
    Iter begin() { return Iter{*this}; }
    Iter end() { return Iter{*this}; }
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

UserFacing<Value> check_divisiable(UserFacing<Value> gen, int divisor, std::string text)
{
    for (auto v : gen)
    {
        if (v.number % divisor == 0)
        {
            v.text.append(text);
        }
        co_yield v;
    }
}

int main()
{
    auto gen = generate_number(10);
    for (auto& v : gen)
    {
        std::cout << v.number << std::endl;
    }
    auto gen2 = generate_number(10);
    auto gen3 = check_divisiable(std::move(gen2), 3, "Fizz");
    auto gen4 = check_divisiable(std::move(gen3), 5, "Buzz");
    for (auto& v : gen4)
    {
        std::cout << v.number << " " << v.text << std::endl;
    }
}
