/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedCallbackQueue.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkThreadedCallbackQueue
 * @brief simple threaded callback queue
 *
 * This callback queue executes pushed functions and functors on threads whose
 * purpose is to execute those functions. When instantiating
 * this class, no threads are spawned yet. They are spawned upon calling `Start()`.
 * By default, one thread is created by this class, so it is advised to set `NumberOfThreads`.
 * Upon destruction of an instance of this callback queue, remaining unexecuted threads are
 * executed, unless `IsRunning()` returns `false`.
 *
 * All public methods of this class are thread safe.
 */

#ifndef vtkThreadedCallbackQueue_h
#define vtkThreadedCallbackQueue_h

#include "vtkObject.h"
#include "vtkParallelCoreModule.h" // For export macro
#include "vtkSmartPointer.h"       // For vtkSmartPointer

#include <atomic>             // For atomic_bool
#include <condition_variable> // For condition variable
#include <memory>             // For unique_ptr
#include <mutex>              // For mutex
#include <queue>              // For queue
#include <thread>             // For thread
#include <vector>             // For vector

#if !defined(__WRAP__)

VTK_ABI_NAMESPACE_BEGIN

class VTKPARALLELCORE_EXPORT vtkThreadedCallbackQueue : public vtkObject
{
public:
  static vtkThreadedCallbackQueue* New();
  vtkTypeMacro(vtkThreadedCallbackQueue, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkThreadedCallbackQueue();

  /**
   * Any remaining function that was not executed yet will be executed in the destructor if
   * `IsRunning()` returns true. In such an instance, the destructor terminates after all functions
   * have been run.
   */
  ~vtkThreadedCallbackQueue() override;

  /**
   * Pushes a function f to be passed args... as arguments.
   * f will be called as soon as a running thread has the occasion to do so, in a FIFO fashion,
   * assuming that `IsRunning()` returns `true`.
   * This method is thread-safe.
   */
  template <class FT, class... ArgsT>
  void Push(FT&& f, ArgsT&&... args);

  /**
   * Sets the number of threads. If the new number of threads is different than the current one,
   * this method calls `Stop()`, so one needs to call `Start()` after the number of threads are set
   * in order to make this queue running.
   *
   * This method is executed by the `Controller` on a different thread, so this method may terminate
   * before the threads were allocated. Nevertheless, this method is thread-safe. Other calls to
   * `SetNumberOfThreads()` will be queued by the `Controller`,
   * which executes all received command serially in the background.
   */
  void SetNumberOfThreads(int numberOfThreads);

  /**
   * Returns the number of allocated threads. Note that this method doesn't give any information on
   * whether threads are running or not.
   *
   * @note `SetNumberOfThreads(int)` runs in the background. So the number of threads of this queue
   * might change asynchronously as those commands are executed.
   */
  int GetNumberOfThreads() const { return this->NumberOfThreads; }

  /**
   * Returns true if the queue is currently running. The running state of this instance is
   * controlled by `Start()` and `Stop()`.
   *
   * @note `Start()` and `Stop()` are running in the background. So the running state of the queue
   * might change asynchronously as those commands are executed.
   */
  bool IsRunning() const { return this->Running; }

  /**
   * Stops the threads as soon as they are done with their current task.
   *
   * This method is executed by the `Controller` on a different thread, so this method may terminate
   * before the threads stopped running. Nevertheless, this method is thread-safe. Other calls to
   * `Stop()` will be queued by the `Controller`, which executes all received command serially in
   * the background. When the `Controller` is done executing this command, `IsRunning()` effectively
   * returns `false`.
   */
  void Stop();

  /**
   * Starts the threads as soon as they are done with their current tasks.
   *
   * This method is executed by the `Controller` on a different thread, so this method may terminate
   * before the threads are spawned. Nevertheless, this method is thread-safe. Other calls to
   * `Start()` will be queued by the `Controller`, which executes all received command serially in
   * the background. When the `Controller` is done executing this command, `IsRunning()` effectively
   * returns `true`.
   */
  void Start();

private:
  ///@{
  /**
   * Worker typedefs that hold the inserted functions and their parameters.
   *
   * `BaseWorker` is the base abstract type that helps us store the queue of workers to execute.
   * Each individual stored worker is actually an instance of `WorkerWrapper` which inherits
   * `BaseWorker`. They share a pure virtual `operator()` that effectively calls the stored workers
   * with the parameters provided.
   */
  struct BaseWorker;
  template <class FT, class... ArgsT>
  class WorkerWrapper;
  ///@}

  class vtkInternalController;
  using WorkerPointer = std::unique_ptr<BaseWorker>;

  /**
   * This method terminates when all threads have finished. If `Destroying` is not true or `Running`
   * is not false, then calling this method results in a deadlock.
   */
  void Sync();

  /**
   * Pops the queue and runs the stored worker.
   */
  void Pop();

  /**
   * Stop routine forced to be run serially. This function doesn't use the `Controller`.
   */
  static void SerialStop(vtkThreadedCallbackQueue* self);

  /**
   * Queue of workers responsible for running the jobs that are inserted.
   */
  std::queue<WorkerPointer> Workers;

  /**
   * This mutex ensures that the queue can pop and push elements in a thread-safe manner.
   */
  std::mutex Mutex;

  std::condition_variable ConditionVariable;

  /**
   * This atomic boolean makes checking if there are workers to process thread-safe.
   */
  std::atomic_bool Empty;

  /**
   * This atomic boolean is false until destruction. It is then used by the workers
   * so they know that they need to terminate when the queue is empty.
   */
  std::atomic_bool Destroying;

  /**
   * This atomic boolean is true when the queue is running, false when the queue is on hold.
   */
  std::atomic_bool Running;

  /**
   * Number of allocated threads. Allocated threads are not necessarily running.
   */
  std::atomic_int NumberOfThreads;

  /**
   * Collection of threads running the jobs.
   */
  std::vector<std::thread> Threads;

  /**
   * The controller is responsible for taking care of the calls to `Stop()`, `Start()`, and
   * `SetNumberOfThreads(int)`. It queues those commands and serially executes them on a separate
   * thread. This allows those methods to not be blocking and run asynchronously.
   */
  vtkSmartPointer<vtkInternalController> Controller;

  vtkThreadedCallbackQueue(const vtkThreadedCallbackQueue&) = delete;
  void operator=(const vtkThreadedCallbackQueue&) = delete;

protected:
  /**
   * Constructor setting internal `Controller` to the provided controller.
   */
  vtkThreadedCallbackQueue(vtkSmartPointer<vtkInternalController>&& controller);
};

VTK_ABI_NAMESPACE_END

#include "vtkThreadedCallbackQueue.txx"

#endif
#endif
// VTK-HeaderTest-Exclude: vtkThreadedCallbackQueue.h
