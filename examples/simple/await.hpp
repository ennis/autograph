#ifndef AWAIT_HPP
#define AWAIT_HPP

#ifdef USE_AWAIT

#include <autograph/error.hpp>
#include <autograph/utils.hpp>
#include <boost/context/execution_context.hpp>
#include <iostream>
#include <rxcpp/rx.hpp>
#include <tuple>

namespace co {

// movable, noncopyable
// resumable task
class task {
public:
  // put the trampoline function into a lambda
  // I got an ICE with the template function initial_resume
  template <typename F, typename... Args>
  task(F&& fn_, Args&&... args_)
      : context(std::allocator_arg, boost::context::fixedsize_stack(), 
		  [](F&& f, Args && ... args,
                   void*) { f(std::forward<Args>(args)...); },
                std::forward<F>(fn_), std::forward<Args>(args_)...),
        next_context(boost::context::execution_context::current()) {
    using namespace boost::context;
    resume();
  }

  void resume() {
    // set our state
    auto tmp = current_task;
    current_task = this;
	std::clog << "resume task\n";
    context();
	std::clog << "return from task\n";
    current_task = tmp;
  }

  static task* current() { return current_task; }
  static void suspend() { 
	  std::clog << "task context: " << current_task->next_context << "\n";
	  std::clog << "current context: " << boost::context::execution_context::current() << "\n";
	  current_task->next_context();
  }

private:
  /*template <typename F, typename... Args>
  static void initial_resume(F&& fn, Args&&... args, void* p)
  {
          std::clog << "Started task\n";
          fn(std::forward<Args>(args)...);
  }*/

  boost::context::execution_context context;
  boost::context::execution_context next_context;
  static thread_local task* current_task;
};

// await: creates a callable that resumes the current coroutine, subscribe to
// the future and suspends the current coroutine
//

// call in a co::task context
template <typename T> T await(rxcpp::observable<T> obs) {
  T value_tmp;
  std::clog << "await enter\n";
  auto ts = task::current();
  if (!ts) {
    std::clog << "ABORT: co::await() called outside a resumable task\n";
    std::terminate();
  }
  auto sub = obs.subscribe([&value_tmp, ts](const T& value) {
    value_tmp = value;
	std::clog << "await callback\n";
    ts->resume();
  });

  // what if the observable triggers here?
  // in this case, the task is not yet suspended
  // and will resume here, causing an abort (in task_state::resume)

  // suspend-resume-point
  std::clog << "await suspending\n";
  task::suspend();
  sub.unsubscribe();
  std::clog << "await exit\n";
  return value_tmp;
}
}

#endif

#endif
