#ifndef DIY_COROUTINE_HPP
#define DIY_COROUTINE_HPP

/*
 * Derived from libco v20 (2019-10-16)
 * author: byuu (https://byuu.org/)
 * license: ISC
 */

namespace diy
{

namespace coroutine
{

using cothread_t = void*;


inline cothread_t  co_active();
inline cothread_t  co_create(unsigned int, void (*)(void));
inline void        co_delete(cothread_t);
inline void        co_switch(cothread_t);

// "global variable" to pass an argument
inline void*&      argument()
{
    static thread_local void* x;
    return x;
}

}

}

#if defined(_WIN32)
#include "coroutine/fiber.hpp"
#else
#include "coroutine/sjlj.hpp"
#endif

#endif
