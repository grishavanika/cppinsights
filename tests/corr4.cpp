#include <experimental/coroutine>
#include <chrono>
#include <iostream>

template<typename T>
struct my_future
{
#if 1
	struct promise_type
	{
		//T current_value;
#if 0        //
		std::experimental::suspend_always yield_value(/*T value*/)
		{
//			this->current_value = value;
			return {};
		}
#endif        
		std::experimental::suspend_always initial_suspend() { return {}; }
		std::experimental::suspend_always final_suspend() { return {}; }
	//	generator get_return_object() { return generator{ this }; };
        auto get_return_object() { return my_future{}; }
		void unhandled_exception() { std::terminate(); }
		void return_void() {}
        // gets us stmt->getReturnStmtOnAllocFailure()
	};
#else
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
    
#endif
    
    bool await_ready();
    void await_suspend(std::experimental::coroutine_handle<>);
    T    await_resume();
};
template<class Rep, class Period>
auto operator co_await(std::chrono::duration<Rep, Period> d)
{
    struct awaiter
    {
        std::chrono::system_clock::duration duration;
        awaiter(std::chrono::system_clock::duration d)
        : duration(d)
        {
        }
        bool await_ready() const { return duration.count() <= 0; }
        void await_resume() {}
        void await_suspend(std::experimental::coroutine_handle<> h) {  }
    };
    return awaiter{d};
}
using namespace std::chrono;

my_future<int>  h();
my_future<int> g()
{
    std::cout << "just about go to sleep...\n";
    co_await 10ms;
    std::cout << "resumed\n";
    co_await h();
}

//auto f(int x = co_await h()); // error: await-expression outside of function suspension context

void bain() {
    co_await g(); // error: await-expression outside of function suspension context
}
