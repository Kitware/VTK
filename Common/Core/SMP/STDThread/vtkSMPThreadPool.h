// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkSMPThreadPool - A thread pool implementation using std::thread
//
// .SECTION Description
// vtkSMPThreadPool class creates a thread pool of std::thread, the number
// of thread must be specified at the initialization of the class.
// The DoJob() method is used attributes the job to a free thread, if all
// threads are working, the job is kept in a queue. Note that vtkSMPThreadPool
// destructor joins threads and finish the jobs in the queue.

#ifndef vtkSMPThreadPool_h
#define vtkSMPThreadPool_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"

#include <atomic>     // For std::atomic
#include <functional> // For std::function
#include <mutex>      // For std::unique_lock
#include <thread>     // For std::thread
#include <vector>     // For std::vector

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief Internal thread pool implementation used in SMP functions
 *
 * This class is designed to be a Singleton thread pool, but local pool can be allocated too.
 * This thread pool use a Proxy system that is used to allocate a certain amount of threads from
 * the pool, which enable support for SMP local scopes.
 * You need to have a Proxy to submit job to the pool.
 */
class VTKCOMMONCORE_EXPORT vtkSMPThreadPool
{
  // Internal data structures
  struct ThreadJob;
  struct ThreadData;
  struct ProxyThreadData;
  struct ProxyData;

public:
  /**
   * @brief Proxy class used to submit work to the thread pool.
   *
   * A proxy act like a single thread pool, but it submits work to its parent thread pool.
   * Using a proxy from multiple threads at the same time is undefined behaviour.
   *
   * Note: Even if nothing prevent a proxy to be moved around threads, it should either be used in
   * the creating thread or in a thread that does not belong to the pool, otherwise it may create a
   * deadlock when joining.
   */
  class VTKCOMMONCORE_EXPORT Proxy final
  {
  public:
    /**
     * @brief Destructor
     *
     * Join must have been called since the last DoJob before destroying the proxy.
     */
    ~Proxy();
    Proxy(const Proxy&) = delete;
    Proxy& operator=(const Proxy&) = delete;
    Proxy(Proxy&&) noexcept;
    Proxy& operator=(Proxy&&) noexcept;

    /**
     * @brief Blocks calling thread until all jobs are done.
     *
     * Note: nested proxies may execute jobs on calling thread during this function to maximize
     * parallelism.
     */
    void Join();

    /**
     * @brief Add a job to the thread pool queue
     */
    void DoJob(std::function<void()> job);

    /**
     * @brief Get a reference on all system threads used by this proxy
     */
    std::vector<std::reference_wrapper<std::thread>> GetThreads() const;

    /**
     * @brief Return true is this proxy is allocated from a thread that does not belong to the pool
     */
    bool IsTopLevel() const noexcept;

  private:
    friend class vtkSMPThreadPool; // Only the thread pool can construct this object

    Proxy(std::unique_ptr<ProxyData>&& data);

    std::unique_ptr<ProxyData> Data;
  };

  vtkSMPThreadPool();
  ~vtkSMPThreadPool();
  vtkSMPThreadPool(const vtkSMPThreadPool&) = delete;
  vtkSMPThreadPool& operator=(const vtkSMPThreadPool&) = delete;

  /**
   * @brief Create a proxy
   *
   * Create a proxy that will use at most threadCount thread of the thread pool.
   * Proxy act as a thread pool on its own, but will in practice submit its work to this pool,
   * this prevent threads to be created every time a SMP function is called.
   *
   * If the current thread not in the pool, it will create a "top-level" proxy, otherwise it will
   * create a nested proxy. A nested proxy will never use a thread that is already in use by its
   * "parent" proxies to prevent deadlocks. It means that nested paralism may have a more limited
   * amount of threads.
   *
   * @param threadCount max amount of thread to use. If 0, uses the number of thread of the pool.
   * If greater than the number of thread of the pool, uses the number of thread of the pool.
   * @return A proxy.
   */
  Proxy AllocateThreads(std::size_t threadCount = 0);

  /**
   * Value returned by `GetThreadID` when called by a thread that does not belong to the pool.
   */
  static constexpr std::size_t ExternalThreadID = 1;

  /**
   * @brief Get caller proxy thread virtual ID
   *
   * This function must be called from a proxy thread.
   * If this function is called from non proxy thread, returns `ExternalThreadID`.
   * Valid proxy thread virtual ID are always >= 2
   */
  std::size_t GetThreadId() const noexcept;

  /**
   * @brief Returns true when called from a proxy thread, false otherwise.
   */
  bool IsParallelScope() const noexcept;

  /**
   * @brief Returns true for a single proxy thread, false for the others.
   */
  bool GetSingleThread() const;

  /**
   * @brief Returns number of system thread used by the thread pool.
   */
  std::size_t ThreadCount() const noexcept;

private:
  // static because also used by proxy
  static void RunJob(ThreadData& data, std::size_t jobIndex, std::unique_lock<std::mutex>& lock);

  ThreadData* GetCallerThreadData() const noexcept;

  std::thread MakeThread();
  void FillThreadsForNestedProxy(ProxyData* proxy, std::size_t maxCount);
  std::size_t GetNextThreadId() noexcept;

  std::atomic<bool> Initialized{};
  std::atomic<bool> Joining{};
  std::vector<std::unique_ptr<ThreadData>> Threads; // Thread pool, fixed size
  std::atomic<std::size_t> NextProxyThreadId{ 1 };

public:
  static vtkSMPThreadPool& GetInstance();
};

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk

#endif
/* VTK-HeaderTest-Exclude: vtkSMPThreadPool.h */
