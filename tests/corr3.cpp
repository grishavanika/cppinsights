#include <experimental/coroutine>

struct auto_await_suspend {
  bool await_ready();
  template <typename F> auto await_suspend(F) {}
  void await_resume();
};

void check_auto_await_suspend() {
  co_await auto_await_suspend{}; // Should compile successfully.
}

