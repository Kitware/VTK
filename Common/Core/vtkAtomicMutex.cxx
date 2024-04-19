// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAtomicMutex.h"

#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#elif (defined(__ARM_FEATURE_SIMD32) || defined(__ARM_NEON)) &&                                    \
  (defined(__ARM_ARCH_ISA_THUMB) && __ARM_ARCH_ISA_THUMB > 1)
// https://github.com/DLTcollab/sse2neon
static inline __attribute__((always_inline)) void _mm_pause()
{
  __asm__ __volatile__("isb\n");
}
#else
#define _mm_pause()
#endif

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkAtomicMutex::vtkAtomicMutex()
  : Locked(false)
{
}

//------------------------------------------------------------------------------
vtkAtomicMutex::vtkAtomicMutex(const vtkAtomicMutex& other)
  : Locked(other.Locked.load(std::memory_order_acquire))
{
}

//------------------------------------------------------------------------------
vtkAtomicMutex& vtkAtomicMutex::operator=(const vtkAtomicMutex& other)
{
  this->Locked.store(other.Locked.load(std::memory_order_acquire), std::memory_order_release);
  return *this;
}

//------------------------------------------------------------------------------
void vtkAtomicMutex::lock()
{
  while (true)
  {
    // The default memory ordering of C++ atomics std::memory_order_seq_cst
    // (sequentially-consistent ordering) is overly restrictive and can be changed
    // to std::memory_order_acquire for operations that acquires the lock and
    // std::memory_order_release for operations that releases the lock2.
    if (!this->Locked.exchange(true, std::memory_order_acquire))
    {
      return;
    }
    // The memory_order_relaxed is used to avoid of continuous futile attempts to acquire
    // the held lock, and we wait for the lock holder to first release the lock. This
    // eliminates cache coherency traffic during spinning:
    while (this->Locked.load(std::memory_order_relaxed))
    {
      // The pause instruction provides a hint that a spin-wait loop is running and
      // throttles the CPU core in some architecture specific way in order to reduce
      // power usage and contention on the load-store units
      _mm_pause();
    }
  }
}

//------------------------------------------------------------------------------
void vtkAtomicMutex::unlock()
{
  this->Locked.store(false, std::memory_order_release);
}
VTK_ABI_NAMESPACE_END
