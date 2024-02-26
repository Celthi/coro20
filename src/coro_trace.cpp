#include <iostream>
#include <coroutine>
#include <optional>
#include <source_location>
#include <fstream>
#include <type_traits>
#include <vector>
#include <string>
#include <map>

std::vector<std::string> g_statuses;

class PlantUML
{
private:
    std::string m_file_name;
    std::ofstream m_file;
    std::vector<std::string> m_participants;
    std::string m_graph_text;

public:
    std::string to_plant_uml()
    {
        std::string result = "@startuml\n";
        for (auto &participant : m_participants)
        {
            result += "participant " + participant + "\n";
        }

        result += "\n";
        result += m_graph_text;
        result += "@enduml\n";
        return result;
    }

    void to_file()
    {
        m_file << to_plant_uml();
    }

    void add_participant(std::string_view participant)
    {
        m_participants.push_back(std::string(participant));
    }
    void note_over(std::string_view note)
    {
        m_graph_text += "note over " + g_statuses.back() + " : " + std::string(note) + "\n";
    }

    void message(std::string_view from, std::string_view to, std::string_view message)
    {
        std::string str = std::string(from) + " -> " + std::string(to) + " : " + std::string(message);
        m_graph_text += str + "\n";
    }
    PlantUML(std::string file_name = "coro_fizz_statuses.puml") : m_file_name(std::move(file_name))
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
};
class StatusEnter
{
public:
    StatusEnter(std::string_view status)
    {
        g_statuses.push_back(std::string(status));
        if (g_statuses.size() > 1)
        {
            std::string message = "Entering " + g_statuses.back() + " from " + g_statuses[g_statuses.size() - 2];
            PlantUML::get_instance().message(g_statuses[g_statuses.size() - 2], g_statuses.back(), message);
        }
    }
    ~StatusEnter()
    {
        g_statuses.pop_back();
        if (g_statuses.size() > 0)
        {
            std::string message = "Leaving " + g_statuses.back() + " to " + g_statuses[g_statuses.size() - 1];
            PlantUML::get_instance().message(g_statuses.back(), g_statuses[g_statuses.size() - 1], message);
        }
    }
};

// class to wrap the coroutine handle to mark status transition
template <typename P>
class CoroHandler
{
public:
    std::string m_name;
    std::coroutine_handle<P> handle;
    CoroHandler(std::string name, std::coroutine_handle<P> h) : m_name(std::move(name)), handle(h) {}
    CoroHandler(const CoroHandler<P> &) = default;
    template <typename T = P>
    CoroHandler(const CoroHandler<std::enable_if_t<!std::is_void_v<T>, void>> &h) : m_name(h.m_name), handle(h.handle) {}
    CoroHandler() = default;
    CoroHandler &operator=(const CoroHandler<P> &) = default;
    CoroHandler(CoroHandler<P> &&s) = default;
    CoroHandler &operator=(CoroHandler<P> &&s) = default;

    template <typename S = P, typename T>
    CoroHandler &operator=(std::enable_if_t<!std::is_void_v<T> && std::is_void_v<S>, T> &&h)
    {
        m_name = h.m_name;
        handle = h.handle;
        return *this;
    }
    void suspend(std::string_view target, std::string_view note = "")
    {
    }

    void resume()
    {
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

    std::conditional_t<std::is_void_v<P>, std::coroutine_handle<>, std::coroutine_handle<P>> get_handle_to_resume(std::string_view from)
    {
        StatusEnter status_enter(from);
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
    std::string m_name;
    CoroHandler<void> consumer_coro_handle;
    YieldAwaitable() : consumer_coro_handle("consume_numbers", nullptr)
    {
        m_name = "YieldAwaitable";
    }
    explicit YieldAwaitable(CoroHandler<void> &h) : m_name("YieldAwaitable"), consumer_coro_handle(h) {}
    constexpr bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept
    {
        // if (consumer_coro_handle)
        // {
        //     return consumer_coro_handle.get_handle_to_resume(m_name);
        // }
        return std::noop_coroutine();
    }
    void await_resume() noexcept
    {
        StatusEnter status_enter("YieldAwaitable");
        PlantUML::get_instance().note_over("YieldAwaitable is about to resume");
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
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h)
        {
            static bool last_one = false;
            StatusEnter status_enter("GenNumberAwaiter");
            // note over
            PlantUML::get_instance().note_over("GenNumberAwaiter is about to resume");
            if (!producer_handler.done())
            {
                producer_handler.get_handle_to_resume("GenNumberAwaiter").resume();
                return h;
            }
            if (!last_one) {
                last_one = true;
                return h;
            }
            return std::noop_coroutine();
        }

        std::optional<Value> await_resume()
        {
            StatusEnter status_enter("GenNumberAwaiter");
            return producer_handler.promise().value;
        }
    };
    // promise_type is the place to store data about the coroutine result
    // and the policy for the coroutine
    struct promise_type
    {
        int limit;
        std::optional<Value> value;
        CoroHandler<void> consumer_coro_handle;
        CoroHandler<promise_type> handle;
        promise_type() = default;
        GenNumber get_return_object()
        {
            this->handle = CoroHandler<promise_type>("generate_numbers", handle_type::from_promise(*this));
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
            StatusEnter status_enter("GenNumber");
            this->handle.suspend("YieldAwaitable", "yield_value");

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
            StatusEnter status_enter("Consumer");
            auto awaitable = GenNumber::GenNumberAwaiter{source.handle};
            source.handle.promise().consumer_coro_handle.operator= <void, CoroHandler<promise_type>>(CoroHandler("consume_numbers", std::coroutine_handle<promise_type>::from_promise(*this)));

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
        StatusEnter status_enter("Consumer");
        if (handle.done())
        {
            return {};
        }
        handle.promise().value = {};
        handle.resume();
        auto v = handle.promise().value;
        return v;
    }
    bool done()
    {
        return handle.done();
    }
};

GenNumber generate_numbers(int limit)
{

    // note over
    PlantUML::get_instance().note_over("generate_numbers is about to start");
    for (int i = 1; i <= limit; i++)
    {
        PlantUML::get_instance().note_over("generate_numbers is about to yield " + std::to_string(i));
        Value v = i;
        co_yield v;
    }
}

Consumer consume_numbers(GenNumber source, int divisor)
{

    StatusEnter status_enter("consume_numbers");
    while (std::optional<Value> vopt = co_await source)
    {
        if (*vopt % divisor == 0)
        {
            co_yield vopt;
        }
    }
}

int main()
{
    PlantUML::get_instance().add_participant("main");
    PlantUML::get_instance().add_participant("consume_numbers");
    PlantUML::get_instance().add_participant("generate_numbers");
    PlantUML::get_instance().add_participant("GenNumberAwaiter");
    PlantUML::get_instance().add_participant("YieldAwaitable");

    GenNumber c = generate_numbers(9);
    auto res = consume_numbers(std::move(c), 6);
    while (std::optional<Value> vopt = res.next_value())
    {
        std::cout << "value: " << *vopt << std::endl;
    }
    PlantUML::get_instance().to_file();
}
