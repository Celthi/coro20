#include <iostream>
#include <coroutine>
#include <optional>
#include <source_location>
#include <fstream>
#include <type_traits>
class PlantUML
{
private:
    std::string m_file_name;
    std::ofstream m_file;

public:
    PlantUML(std::string file_name = "coro_fizz.puml") : m_file_name(std::move(file_name))
    {
        m_file.open(m_file_name);
        if (!m_file.is_open())
        {
            std::cerr << "Failed to open file " << m_file_name << std::endl;
        }
    }
    ~PlantUML()
    {
        if (m_file.is_open())
        {
            m_file.close();
        }
    }
    PlantUML(const PlantUML &) = delete;
    PlantUML &operator=(const PlantUML &) = delete;
    PlantUML(PlantUML &&) = delete;
    PlantUML &operator=(PlantUML &&) = delete;
    static PlantUML &get_instance()
    {
        static PlantUML instance;
        return instance;
    }
    void startuml()
    {
        std::cout << "@startuml" << std::endl;
        m_file << "@startuml" << std::endl;
    }
    void enduml()
    {
        std::cout << "@enduml" << std::endl;
        m_file << "@enduml" << std::endl;
    }
    void note_right(const std::string &note)
    {
        std::cout << "note right\n"
                  << note << " \nend note\n"
                  << std::endl;
        m_file << "note right\n"
               << note << " \nend note\n"
               << std::endl;
    }

    void note_over(const std::string &note, const std::string &participant)
    {
        std::cout << "note over " << participant << "\n"
                  << note << " \nend note\n"
                  << std::endl;
        m_file << "note over " << participant << "\n"
               << note << " \nend note\n"
               << std::endl;
    }

    void add_participant(const std::string &participant)
    {
        std::cout << "participant " << participant << std::endl;
        m_file << "participant " << participant << std::endl;
    }
    void message(const std::string &from, const std::string &to, const std::string &message)
    {
        std::cout << from << " -> " << to << " : " << message << std::endl;
        m_file << from << " -> " << to << " : " << message << std::endl;
    }
    void message(const std::string &from, const std::string &to, const std::string &message, const std::string &note)
    {
        this->message(from, to, message);
        note_right(note);
    }
};

// class to wrap the coroutine handle to mark status transition
template <typename P>
class CoroHandler
{
public:
    std::string m_name;
    std::coroutine_handle<P> handle;
    std::vector<std::string> m_transfer_targets;
    CoroHandler(std::string name, std::coroutine_handle<P> h) : m_name(std::move(name)), handle(h) {}
    //CoroHandler(const CoroHandler<P> &) = default;
    CoroHandler(const CoroHandler<std::conditional_t<std::is_void_v<P>, std::coroutine_handle<>, std::coroutine_handle<P>>>& h) : m_name(h.m_name), handle(h.handle) {}
    
    CoroHandler &operator=(const CoroHandler<P> &) = default;
    CoroHandler(CoroHandler<P> &&s) = default;
    CoroHandler &operator=(CoroHandler<P> &&s) = default;

    void suspend(std::string target)
    {
        PlantUML::get_instance().message(m_name, target, "suspend()");
        m_transfer_targets.push_back(target);
    }

    void resume()
    {
        PlantUML::get_instance().message(m_transfer_targets.back(), m_name, "resume()");
        handle.resume();
    }

    bool done()
    {
        return handle.done();
    }

    void destroy()
    {
        if (handle)
            handle.destroy();
    }

    explicit operator bool() const
    {
        return handle != nullptr;
    }

    std::conditional_t<std::is_void_v<P>, std::coroutine_handle<>, std::coroutine_handle<P>> get_handle(const std::string &from, const std::string &msg)
    {
        PlantUML::get_instance().message(from, m_name, msg);
        return handle;
    }

    // if P is not void then define promise method
    template <typename T = P>
    std::enable_if_t<!std::is_void_v<T>, std::add_lvalue_reference_t<T>> promise()
    {
        return handle.promise();
    }
};

struct YieldAwaitable
{
    CoroHandler<void> consumer_coro_handle;
    std::string m_name;
    YieldAwaitable() : consumer_coro_handle("null", nullptr)
    {
        m_name = "YieldAwaitable";
    }
    explicit YieldAwaitable(std::coroutine_handle<> h) : consumer_coro_handle("consume_numbers", h) {}
    constexpr bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept
    {
        if (consumer_coro_handle)
        {
            return consumer_coro_handle.get_handle(m_name, "await_suspend");
        }
        return std::noop_coroutine();
    }
    void await_resume() noexcept
    {
        PlantUML::get_instance().message("YieldAwaitable", "consume_numbers", "await_resume");
    }
};

using Value = int;
// The coroutine that generates numbers
class GenNumber
{
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    CoroHandler<promise_type> handle; // the coroutine handle
    // rule of zero
    GenNumber() = delete;
    GenNumber(const GenNumber &) = delete;                        // 1. no copy constructor
    GenNumber &operator=(const GenNumber &) = delete;             // 2. no copy assignment
    explicit GenNumber(handle_type h) : handle("GenNumber", h) {} // 3. constructor
    ~GenNumber()                                                  // 4. destructor
    {
        if (handle)
        {
            handle.destroy();
        }
    }
    GenNumber(GenNumber &&s) : handle("GenNumber", s.handle.handle) // 5. move constructor
    {
        s.handle.handle = nullptr;
    }
    GenNumber &operator=(GenNumber &&s) // 6. move assignment
    {
        handle = std::move(s.handle);
        return *this;
    }

    class GenNumberAwaiter
    {
    public:
        CoroHandler<promise_type> producer_handler;
        explicit GenNumberAwaiter(const CoroHandler<promise_type> &p) : producer_handler("generate_numbers", p.handle) {}
        GenNumberAwaiter(const GenNumberAwaiter &) = default;
        GenNumberAwaiter &operator=(const GenNumberAwaiter &) = default;
        GenNumberAwaiter(GenNumberAwaiter &&) = default;
        GenNumberAwaiter &operator=(GenNumberAwaiter &&) = default;
        bool await_ready() const { return false; }
        std::coroutine_handle<promise_type> await_suspend(std::coroutine_handle<>)
        {
            PlantUML::get_instance().note_over("GenNumberAwaiter::await_suspend", "GenNumberAwaiter");
            PlantUML::get_instance().message("GenNumberAwaiter", "generate_numbers", "producer_handler.resume()");
            return producer_handler.get_handle("GenNumberAwaiter", "await_suspend");
        }

        std::optional<Value> await_resume()
        {
            PlantUML::get_instance().message("generate_numbers", "GenNumberAwaiter", "await_resume");
            PlantUML::get_instance().message("GenNumberAwaiter", "consume_numbers", "await_resume");
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
            
            PlantUML::get_instance().message("generate_numbers", "YieldAwaitable", "yield_value");
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
        std::optional<Value> value;                             // the value to be returned
        CoroHandler<GenNumber::promise_type> &producer_handler; // the producer coroutine handle
        promise_type(GenNumber &source, int divisor) : producer_handler(source.handle) {}
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
        GenNumber::GenNumberAwaiter await_transform(GenNumber &source)
        {
            PlantUML::get_instance().message("consume_numbers", "GenNumberAwaiter", "await_transform");
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
        handle.promise().producer_handler.promise().value = {};
        handle.resume();
        auto v = handle.promise().producer_handler.promise().value;
        return v;
        return {};
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
        PlantUML::get_instance().note_over("generate_numbers is about to yield " + std::to_string(i), "generate_numbers");
        Value v = i;
        co_yield v;
    }
}

Consumer consume_numbers(GenNumber source, int divisor)
{
    PlantUML::get_instance().note_over("consume_numbers is about to call await_transform", "consume_numbers");
    while (std::optional<Value> vopt = co_await source) // Consumer::await_transform -> GenNumberAwaiter::await_suspend ->generate_numbers::resume() -> *yieldawaitable::await_suspend* -> consumer_coro_handle.resume() -> GenNumberAwaiter::await_resume
    {
        PlantUML::get_instance().message("consume_numbers", "GenNumberAwaiter", "co_await");

        PlantUML::get_instance().note_over("consume_numbers co_await result = " + std::to_string(*vopt), "consume_numbers");
        if (*vopt % divisor == 0)
        {
            co_yield vopt;
        }
        PlantUML::get_instance().note_over("consume_numbers is about to co_await next value", "consume_numbers");
    }
    PlantUML::get_instance().note_over("consume_numbers end...", "consume_numbers");
}

int main()
{
    PlantUML::get_instance().startuml();
    PlantUML::get_instance().add_participant("main");
    PlantUML::get_instance().add_participant("consume_numbers");
    PlantUML::get_instance().add_participant("generate_numbers");
    PlantUML::get_instance().add_participant("GenNumberAwaiter");
    PlantUML::get_instance().add_participant("YieldAwaitable");

    GenNumber c = generate_numbers(1);
    auto res = consume_numbers(std::move(c), 1);
    PlantUML::get_instance().message("main", "consume_numbers", "consume_numbers.next_value()");
    while (std::optional<Value> vopt = res.next_value())
    {
        PlantUML::get_instance().note_over("main: consume_numbers next value = " + std::to_string(*vopt), "main");
    }
    PlantUML::get_instance().enduml();
}
