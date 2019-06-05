#ifndef ZFP_MEMORY_H
#define ZFP_MEMORY_H

#ifdef _WIN32
extern "C" {
  #ifdef __MINGW32__
  #include <x86intrin.h>
  #endif

  #include <malloc.h>
}
#endif

#include <algorithm>
#include <cstdlib>
#include "zfp/types.h"

#define unused_(x) ((void)(x))

namespace zfp {

// allocate size bytes
inline void*
allocate(size_t size)
{
  return new uchar[size];
}

// allocate size bytes with alignment
inline void*
allocate_aligned(size_t size, size_t alignment)
{
  void* ptr;
  bool is_mem_failed = false;

#ifdef ZFP_WITH_ALIGNED_ALLOC
  #ifdef __INTEL_COMPILER
  ptr = _mm_malloc(size, alignment);

  #elif defined(__MINGW32__)
  ptr = __mingw_aligned_malloc(size, alignment);

  #elif defined(_WIN32)
  ptr = _aligned_malloc(size, alignment);

  #elif (_POSIX_C_SOURCE >= 200112L) || (_XOPEN_SOURCE >= 600) || defined(__MACH__)
  is_mem_failed = posix_memalign(&ptr, alignment, size);

  #else
  unused_(alignment);
  ptr = malloc(size);

  #endif

#else
  unused_(alignment);
  ptr = malloc(size);

#endif

  if (is_mem_failed || (ptr == NULL))
    throw std::bad_alloc();

  return ptr;
}

// deallocate memory pointed to by ptr
template <typename T>
inline void
deallocate(T* ptr)
{
  delete[] ptr;
}

template <typename T>
inline void
deallocate_aligned(T* ptr)
{
#ifdef ZFP_WITH_ALIGNED_ALLOC
  if (ptr)
  #ifdef __INTEL_COMPILER
    _mm_free(ptr);
  #elif defined(__MINGW32__)
    __mingw_aligned_free(ptr);
  #elif defined(_WIN32)
    _aligned_free(ptr);
  #else
    free(ptr);
  #endif

#else
  if (ptr)
    free(ptr);
#endif
}

// reallocate size bytes
template <typename T>
inline void
reallocate(T*& ptr, size_t size)
{
  zfp::deallocate(ptr);
  ptr = static_cast<T*>(zfp::allocate(size));
}

template <typename T>
inline void
reallocate_aligned(T*& ptr, size_t size, size_t alignment)
{
  zfp::deallocate_aligned(ptr);
  ptr = static_cast<T*>(zfp::allocate_aligned(size, alignment));
}

// clone array 'T src[count]'
template <typename T>
inline void
clone(T*& dst, const T* src, size_t count)
{
  zfp::deallocate(dst);
  if (src) {
    dst = static_cast<T*>(zfp::allocate(count * sizeof(T)));
    std::copy(src, src + count, dst);
  }
  else
    dst = 0;
}

template <typename T>
inline void
clone_aligned(T*& dst, const T* src, size_t count, size_t alignment)
{
  zfp::deallocate_aligned(dst);
  if (src) {
    dst = static_cast<T*>(zfp::allocate_aligned(count * sizeof(T), alignment));
    std::copy(src, src + count, dst);
  }
  else
    dst = 0;
}

}

#undef unused_

#endif
