#ifndef ZFP_MEMORY_H
#define ZFP_MEMORY_H

#include <algorithm>
#include <cstdlib>
#include "zfp/types.h"

// allocate size bytes with optional alignment
inline void*
allocate(size_t size, size_t alignment = 0)
{
#if defined(__USE_XOPEN2K) && defined(ZFP_WITH_ALIGNED_ALLOC)
  void* ptr;
  if (alignment > 1)
    posix_memalign(&ptr, alignment, size);
  else
    ptr = malloc(size);
  return ptr;
#else
  return new uchar[size];
#endif
}

// deallocate memory pointed to by ptr
template <typename T>
inline void
deallocate(T* ptr)
{
#if defined(__USE_XOPEN2K) && defined(ZFP_WITH_ALIGNED_ALLOC)
  if (ptr)
    free(ptr);
#else
  delete[] ptr;
#endif
}

// reallocate size bytes with optional alignment
template <typename T>
inline void
reallocate(T*& ptr, size_t size, size_t alignment = 0)
{
  deallocate(ptr);
  ptr = static_cast<T*>(allocate(size, alignment));
}

// clone array 'T src[count]' with optional alignment
template <typename T>
inline void
clone(T*& dst, const T* src, size_t count, size_t alignment = 0)
{
  deallocate(dst);
  if (src) {
    dst = static_cast<T*>(allocate(count * sizeof(T), alignment));
    std::copy(src, src + count, dst);
  }
  else
    dst = 0;
}

#endif
