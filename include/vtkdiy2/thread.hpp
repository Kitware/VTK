#ifndef DIY_THREAD_H
#define DIY_THREAD_H

#ifdef DIY_NO_THREADS
#include "no-thread.hpp"
#else

#include "thread/fast_mutex.h"

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

#endif

#include "critical-resource.hpp"

#endif
