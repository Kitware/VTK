/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  std::mutex emulation
 * Author:   Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (c) 2021, Even Rouault <even dot rouault at spatialys dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "proj/util.hpp"
#include "proj_internal.h"

#ifndef __MINGW32__
#include <mutex>
#endif

NS_PROJ_START

#ifdef __MINGW32__
// mingw32-win32 doesn't implement std::mutex
class mutex {
  public:
    // cppcheck-suppress functionStatic
    void lock() { pj_acquire_lock(); }
    // cppcheck-suppress functionStatic
    void unlock() { pj_release_lock(); }
};

template <class Lock> struct lock_guard {
    Lock &lock_;
    lock_guard(Lock &lock) : lock_(lock) { lock_.lock(); }
    ~lock_guard() { lock_.unlock(); }
};

#else

typedef std::mutex mutex;
template <class Mutex> using lock_guard = std::lock_guard<Mutex>;

#endif

NS_PROJ_END
