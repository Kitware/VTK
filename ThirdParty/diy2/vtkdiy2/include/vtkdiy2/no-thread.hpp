#ifndef DIY_NO_THREAD_HPP
#define DIY_NO_THREAD_HPP

#include <utility>
#include <functional>
#include <type_traits>

// replicates only the parts of the threading interface that we use
// executes everything in a single thread
namespace diy
{
  struct thread
  {
                        thread()                                  {}
                        thread(thread&&)                          = default;
                        thread(const thread&)                     = delete;

    template<class Function, class... Args>
    explicit            thread(Function&& f, Args&&... args)      { f(args...); }       // not ideal, since it doesn't support member functions

    void                join()                                    {}

    static unsigned     hardware_concurrency()                    { return 1; }
  };

  struct mutex {};
  struct fast_mutex {};
  struct recursive_mutex {};

  template<class T>
  struct lock_guard
  {
      lock_guard(T&)        {}
  };

  namespace this_thread
  {
      inline unsigned long int  get_id()    { return 0; }
  }
}

#endif
