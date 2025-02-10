#ifndef DIY_THREAD_H
#define DIY_THREAD_H

#include <map>

#ifdef DIY_NO_THREADS
#include "no-thread.hpp"
#else

#if !defined(_MSC_VER)
#include "thirdparty/thread/fast_mutex.h"
#endif

#include <thread>
#include <mutex>

namespace diy
{
    using std::thread;
    using std::mutex;
    using std::recursive_mutex;
    namespace this_thread = std::this_thread;

#if defined(_MSC_VER)
    // fast_mutex implementation has issues on MSVC. Just use std::mutex
    using fast_mutex = std::mutex;
#else
    // TODO: replace with our own implementation using std::atomic_flag
    using fast_mutex = tthread::fast_mutex;
#endif

    template<class Mutex>
    using lock_guard = std::unique_lock<Mutex>;

    template<class T, class U>
    struct concurrent_map;
}

#endif // DIY_NO_THREADS

#include "critical-resource.hpp"

#if !defined(DIY_NO_THREADS)

#include <memory>       // for shared_ptr

template<class T, class U>
struct diy::concurrent_map
{
    using Map       = std::map<T,U>;
    using SharedPtr = std::shared_ptr<lock_guard<fast_mutex>>;

    template<class MapIterator>
    struct iterator_
    {
        MapIterator     it;
        SharedPtr       lock_ptr;

                        iterator_(const MapIterator& it_, const SharedPtr& lock_ptr_ = SharedPtr()):
                            it(it_), lock_ptr(lock_ptr_)                        {}

        iterator_&      operator++()        { ++it; return *this; }
        iterator_       operator++(int)     { iterator_ retval = *this; ++(*this); return retval; }

        bool            operator==(const iterator_& other) const     { return it == other.it;}
        bool            operator!=(const iterator_& other) const     { return !(*this == other); }

        decltype(*it)               operator*() const   { return *it; }
        decltype(it.operator->())   operator->() const  { return it.operator->(); }
    };

    using iterator       = iterator_<typename Map::iterator>;
    using const_iterator = iterator_<typename Map::const_iterator>;

    U&              operator[](const T& x)  { lock_guard<fast_mutex> l(mutex_); return map_[x]; }

    iterator        begin()                 { auto p = std::make_shared<lock_guard<fast_mutex>>(mutex_); return iterator(map_.begin(), p); }
    iterator        end()                   { return iterator(map_.end()); }

    const_iterator  begin() const           { auto p = std::make_shared<lock_guard<fast_mutex>>(mutex_); return const_iterator(map_.begin(), p); }
    const_iterator  end() const             { return const_iterator(map_.end()); }

    iterator        find(const T& x)        { auto p = std::make_shared<lock_guard<fast_mutex>>(mutex_); return iterator(map_.find(x), p); }
    const_iterator  find(const T& x) const  { auto p = std::make_shared<lock_guard<fast_mutex>>(mutex_); return const_iterator(map_.find(x), p); }

    void            clear()                 { lock_guard<fast_mutex> l(mutex_); map_.clear(); }
    bool            empty()                 { lock_guard<fast_mutex> l(mutex_); return map_.empty(); }

    Map                 map_;
    mutable fast_mutex  mutex_;
};
#endif // !defined(DIY_NO_THREADS)

#endif
