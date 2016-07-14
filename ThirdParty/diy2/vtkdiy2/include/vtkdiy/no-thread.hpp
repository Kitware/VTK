#ifndef DIY_NO_THREAD_HPP
#define DIY_NO_THREAD_HPP

// replicates only the parts of the threading interface that we use
// executes everything in a single thread

namespace diy
{
  struct thread
  {
                        thread(void (*f)(void *), void* args):
                            f_(f), args_(args)                    {}

    void                join()                                    { f_(args_); }

    static unsigned     hardware_concurrency()                    { return 1; }

    void (*f_)(void*);
    void*   args_;
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
