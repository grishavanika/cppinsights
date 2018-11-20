// https://wandbox.org/permlink/Xb1Lu7DMmm1NVkNC
#include <experimental/coroutine>
#include <variant>
#include <mutex>
#include <condition_variable>
#include <stdio.h>

namespace std::experimental {

template <typename T = void>
struct [[nodiscard]] task {
  struct promise_type {
    variant<monostate, T, exception_ptr> result;
    coroutine_handle<> waiter; // who waits on this coroutine
    
    auto get_return_object() { return task{*this}; }
    void return_value(T value) { result.template emplace<1>(std::move(value)); }
    void unhandled_exception() { result.template emplace<2>(std::current_exception()); }
    suspend_always initial_suspend() { return {}; }
    auto final_suspend() {
      struct final_awaiter {
        bool await_ready() { return false; }
        void await_resume() {}
        auto await_suspend(coroutine_handle<promise_type> me) {
          return me.promise().waiter;
        }
      };
      return final_awaiter{};
    }
  };
  
  task(task&& rhs) : h(rhs.h) { rhs.h = nullptr; }
  ~task() { if (h) h.destroy(); }
  explicit task(promise_type& p) : h(coroutine_handle<promise_type>::from_promise(p)) {}
  
  bool await_ready() { return false; }
  T await_resume() {  
    auto& result = h.promise().result;
    if (result.index() == 1) return get<1>(result);
    rethrow_exception(get<2>(result));
  }
  void await_suspend(coroutine_handle<> waiter) {
    h.promise().waiter = waiter;
    h.resume();
  }
  auto _secret() { return h; } // to help sync_await
  
private:
  coroutine_handle<promise_type> h;
};

}

using namespace std::experimental;

task<int> f() { co_return 42; }
task<int> g() { co_return co_await f() + co_await f(); }

template <typename Whatever> // here is a quick hack of universal sync_await
auto sync_await(Whatever x) {
  if (!x.await_ready()) {
    std::mutex m;
    std::condition_variable c;
    bool done = false;
    // helper to hookup continuation to whatever x is
    auto helper = [&]()->task<char> {
      { std::lock_guard grab(m); done = true; }
      c.notify_one();
      co_return 1; // I was lazy and did not create task<void> in this sample
    };
    auto helper_task = helper();
    auto helper_handle = helper_task._secret();
    // we wait for helper using mutex + condition variable, no need to resume anything
    // hence, hacking the waiter to helper to be a noop_coroutine
    helper_handle.promise().waiter = noop_coroutine();
    
    x.await_suspend(helper_handle); // tell x to resume helper when done
    
    std::unique_lock lk(m); // block until x completes
    if (!done) c.wait(lk, [&]{ return done; }); 
  }
  return x.await_resume();    
}

int main() {
    puts("Hello");
    printf("%d\n", sync_await(f()));
    printf("%d\n", sync_await(g()));
}
