#include <coroutine>
#include <iostream>

class UserFacing {
  public:
    class promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    class promise_type {
      public:
        UserFacing get_return_object() {
            auto handle = handle_type::from_promise(*this);
            return UserFacing{handle};
        }
        std::suspend_always initial_suspend() { return {}; }
        void return_void() {}
        void unhandled_exception() {}
        std::suspend_always final_suspend() noexcept { return {}; }
    };

  private:
    handle_type handle;

    UserFacing(handle_type handle) : handle(handle) {}

    UserFacing(const UserFacing &) = delete;
    UserFacing &operator=(const UserFacing &) = delete;

  public:
    bool resume() {
        if (!handle.done())
            handle.resume();
        return !handle.done();
    }

    UserFacing(UserFacing &&rhs) : handle(rhs.handle) {
        rhs.handle = nullptr;
    }
    UserFacing &operator=(UserFacing &&rhs) {
        if (handle)
            handle.destroy();
        handle = rhs.handle;
        rhs.handle = nullptr;
        return *this;
    }
    ~UserFacing() {
        handle.destroy();
    }

    friend class SuspendOtherAwaiter;  // so it can get the handle
};

class TrivialAwaiter {
  public:
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<>) {}
    void await_resume() {}
};

class ReadyTrueAwaiter {
  public:
    bool await_ready() { return true; }
    void await_suspend(std::coroutine_handle<>) {}
    void await_resume() {}
};

class SuspendFalseAwaiter {
  public:
    bool await_ready() { return false; }
    bool await_suspend(std::coroutine_handle<>) { return false; }
    void await_resume() {}
};

class SuspendTrueAwaiter {
  public:
    bool await_ready() { return false; }
    bool await_suspend(std::coroutine_handle<>) { return true; }
    void await_resume() {}
};

class SuspendSelfAwaiter {
  public:
    bool await_ready() { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) {
        return h;
    }
    void await_resume() {}
};

class SuspendNoopAwaiter {
  public:
    bool await_ready() { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) {
        return std::noop_coroutine();
    }
    void await_resume() {}
};

class SuspendOtherAwaiter {
    std::coroutine_handle<> handle;
  public:
    SuspendOtherAwaiter(UserFacing &uf) : handle(uf.handle) {}
    bool await_ready() { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) {
        return handle;
    }
    void await_resume() {}
};

UserFacing demo_coroutine(UserFacing &aux_instance) {
    std::cout << "TrivialAwaiter:" << std::endl;
    co_await TrivialAwaiter{};
    std::cout << "ReadyTrueAwaiter:" << std::endl;
    co_await ReadyTrueAwaiter{};
    std::cout << "SuspendFalseAwaiter:" << std::endl;
    co_await SuspendFalseAwaiter{};
    std::cout << "SuspendTrueAwaiter:" << std::endl;
    co_await SuspendTrueAwaiter{};
    std::cout << "SuspendSelfAwaiter:" << std::endl;
    co_await SuspendSelfAwaiter{};
    std::cout << "SuspendNoopAwaiter:" << std::endl;
    co_await SuspendNoopAwaiter{};
    std::cout << "SuspendOtherAwaiter:" << std::endl;
    co_await SuspendOtherAwaiter{aux_instance};
    std::cout << "goodbye from coroutine" << std::endl;
}

UserFacing aux_coroutine() {
    while (true) {
        std::cout << "  aux_coroutine was resumed" << std::endl;
        co_await std::suspend_always{};
    }
}

int main() {
    UserFacing aux_instance = aux_coroutine();
    UserFacing demo_instance = demo_coroutine(aux_instance);
    while (demo_instance.resume())
        std::cout << "  suspended and came back to main()" << std::endl;
    std::cout << "and it's goodbye from main()" << std::endl;
}
