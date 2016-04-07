#ifndef DIY_THREAD_H
#define DIY_THREAD_H

#ifdef DIY_NO_THREADS
#include "no-thread.hpp"
#else

#include "thread/fast_mutex.h"

#if __cplusplus > 199711L           // C++11
#include <thread>
#include <mutex>

namespace diy
{
    using std::thread;
    using std::mutex;
    using std::recursive_mutex;
    namespace this_thread = std::this_thread;

    // TODO: replace with our own implementation using std::atomic_flag
    using fast_mutex = tthread::fast_mutex;

    template<class Mutex>
    using lock_guard = std::unique_lock<Mutex>;
}

#else
#include "thread/tinythread.h"

namespace diy
{
  using tthread::thread;
  using tthread::mutex;
  using tthread::fast_mutex;
  using tthread::recursive_mutex;
  using tthread::lock_guard;
  namespace this_thread = tthread::this_thread;
}
#endif
#endif

#include "critical-resource.hpp"

#endif
