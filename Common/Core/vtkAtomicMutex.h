// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAtomicMutex
 * @brief   mutual exclusion locking class using atomic operations
 *
 * vtkAtomicMutex allows the locking of variables which are accessed
 * through different threads using atomic operations. An atomic mutex
 * might be preferable over std::mutex, because it is faster when you want to spin lock and the
 * probability of acquiring the lock is high. The benefit of vtkAtomicMutex over
 * std::atomic<bool> is that it is copy constructible, and that is has predefined optimized
 * lock/unlock functions that can be used as a drop in replacement instead of std::mutex.
 */

#ifndef vtkAtomicMutex_h
#define vtkAtomicMutex_h

#include "vtkCommonCoreModule.h" // For export macro
#include <atomic>                // For std::atomic

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONCORE_EXPORT vtkAtomicMutex
{
public:
  // left public purposely to allow for copy construction
  vtkAtomicMutex();
  ~vtkAtomicMutex() = default;
  vtkAtomicMutex(const vtkAtomicMutex& other);
  vtkAtomicMutex& operator=(const vtkAtomicMutex& other);

  ///@{
  /**
   * Lock/Unlock atomic operation.
   *
   * Note: lock/unlock is lowercase, to conform to the mutex wrapper std::lock_guard
   * which provides a convenient RAII-style mechanism  for owning a mutex for the
   * duration of a scoped block.
   */
  void lock();
  void unlock();
  ///@}
private:
  std::atomic_bool Locked;
};

VTK_ABI_NAMESPACE_END
#endif // vtkAtomicMutex_h
// VTK-HeaderTest-Exclude: vtkAtomicMutex.h
