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
 * purpose is to execute those functions.
 * By default, one thread is created by this class, so it is advised to set `NumberOfThreads`.
 * Upon destruction of an instance of this callback queue, remaining unexecuted threads are
 * executed.
 *
 * When a task is pushed, a `vtkSharedFuture` is returned. This instance can be used to get the
 * returned value when the task is finished, and provides functionalities to synchronize the main
 * thread with the status of its associated task. Note that a `vtkSharedFuture` should be cloned if
 * used in different threads.
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
#include <deque>              // For deque
#include <functional>         // For greater
#include <memory>             // For unique_ptr
#include <mutex>              // For mutex
#include <thread>             // For thread
#include <unordered_map>      // For unordered_map
#include <unordered_set>      // For unordered_set
#include <vector>             // For vector

#if !defined(__WRAP__)

VTK_ABI_NAMESPACE_BEGIN

class VTKPARALLELCORE_EXPORT vtkThreadedCallbackQueue : public vtkObject
{
private:
  /**
   * Helper to extract the parameter types of a function (passed by template)
   */
  template <class FT>
  struct Signature;

  /**
   * Helper that dereferences the input type `T`. If `T == Object`, or
   * `T == Object*` or `T == std::unique_ptr<Object>`, then `Type` is of type `Object`.
   */
  template <class T, class DummyT = std::nullptr_t>
  struct Dereference
  {
    struct Type;
  };

  /**
   * Convenient typedef to help lighten the code.
   */
  template <class T>
  using DereferencedType = typename std::decay<typename Dereference<T>::Type>::type;

  /**
   * This resolves to the return type of the template function `FT`.
   */
  template <class FT>
  using InvokeResult = typename Signature<DereferencedType<FT>>::InvokeResult;

public:
  static vtkThreadedCallbackQueue* New();
  vtkTypeMacro(vtkThreadedCallbackQueue, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkThreadedCallbackQueue();

  /**
   * Any remaining function that was not executed yet will be executed in this destructor.
   */
  ~vtkThreadedCallbackQueue() override;

  struct InvokerFutureSharedStateBase;
  template <class ReturnT, class DummyT = void>
  struct InvokerFutureSharedState;

  /**
   * A `vtkSharedFutureBase` is an object returned by the methods `Push` and `PushDependent`.
   * It provides a few functionalities to allow one to synchronize tasks.
   * This future is associated with the task that was pushed.
   */
  class vtkSharedFutureBase : public vtkObjectBase
  {
  public:
    vtkBaseTypeMacro(vtkSharedFutureBase, vtkObjectBase);

    vtkSharedFutureBase() = default;

    /**
     * Blocks current thread until the task associated with this future has terminated.
     */
    virtual void Wait() const = 0;

    friend class vtkThreadedCallbackQueue;

  private:
    /**
     * Returns the shared state owned by this instance. The shared state is instantiated in the
     * children of this class.
     */
    virtual InvokerFutureSharedStateBase* GetSharedState() = 0;
    virtual const InvokerFutureSharedStateBase* GetSharedState() const = 0;

    vtkSharedFutureBase(const vtkSharedFutureBase& other) = delete;
    void operator=(const vtkSharedFutureBase& other) = delete;
  };

  /**
   * A `vtkSharedFuture` is an object returned by the methods `Push` and `PushDependent`.
   */
  template <class ReturnT>
  class vtkSharedFuture : public vtkSharedFutureBase
  {
  public:
    static vtkSharedFuture<ReturnT>* New();
    vtkTypeMacro(vtkSharedFuture<ReturnT>, vtkSharedFutureBase);

    using ReturnLValueRef = typename InvokerFutureSharedState<ReturnT>::ReturnLValueRef;
    using ReturnConstLValueRef = typename InvokerFutureSharedState<ReturnT>::ReturnConstLValueRef;

    vtkSharedFuture()
      : SharedState(std::make_shared<InvokerFutureSharedState<ReturnT>>())
    {
    }

    void Wait() const override;

    /**
     * This returns the return value of the pushed function.
     * It returns a `ReturnT&` if `ReturnT` is not `void`. Returns `void` otherwise.
     */
    ReturnLValueRef Get();

    /**
     * This returns the return value of the pushed function.
     * It returns a `const ReturnT&` if `ReturnT` is not `void`. Returns `void` otherwise.
     */
    ReturnConstLValueRef Get() const;

    friend class vtkThreadedCallbackQueue;

  private:
    InvokerFutureSharedStateBase* GetSharedState() override;
    const InvokerFutureSharedStateBase* GetSharedState() const override;

    std::shared_ptr<InvokerFutureSharedState<ReturnT>> SharedState;

    vtkSharedFuture(const vtkSharedFuture<ReturnT>& other) = delete;
    void operator=(const vtkSharedFuture<ReturnT>& other) = delete;
  };

  using SharedFutureBasePointer = vtkSmartPointer<vtkSharedFutureBase>;
  template <class ReturnT>
  using SharedFuturePointer = vtkSmartPointer<vtkSharedFuture<ReturnT>>;

  /**
   * Pushes a function f to be passed args... as arguments.
   * f will be called as soon as a running thread has the occasion to do so, in a FIFO fashion.
   * This method returns a `SharedFuture`, which is an object
   * allowing to synchronize the code.
   *
   * All the arguments of `Push` are stored persistently inside the queue. An argument passed as an
   * lvalue reference will trigger a copy constructor call. It is thus advised, when possible, to
   * pass rvalue references or smart pointers (`vtkSmartPointer` or `std::shared_ptr` for example)
   *
   * The input function can be a pointer function, a lambda expression, a `std::function`
   * a functor or a member function pointer.
   *
   * If f is a functor, its copy constructor will be invoked if it is passed as an lvalue reference.
   * Consequently, if the functor is somewhat heavy, it is adviced to pass it as an rvalue reference
   * or to wrap inside a smart pointer (`std::unique_ptr` for example).
   *
   * If f is a member function pointer, an instance of its host class needs to be provided in the
   * second parameter. Similar to the functor case, it is advised in order to avoid calling the copy
   * constructor to pass it as an rvalue reference or to wrap it inside a smart pointer.
   *
   * Below is a short example showing off different possible insertions to this queue:
   *
   * @code
   *   struct S {
   *    void operator()(int) {}
   *    void f() {}
   *    void f_const() {}
   *   };
   *   void f(S&&, const S&) {}
   *
   *   vtkNew<vtkThreadedCallbackQueue> queue;
   *   int x;
   *   S s;
   *
   *   // Pushing a lambda expression.
   *   queue->Push([]{});
   *
   *   // Pushing a function pointer
   *   // Note that the copy constructor is called for s
   *   queue->Push(&f, s, S());
   *
   *   // Pushing a functor.
   *   queue->Push(S(), x);
   *
   *   // Pushing a functor wrapped inside a smart pointer.
   *   queue->Push(std::unique_ptr<S>(new S()), x);
   *
   *   // Pushing a member function pointer.
   *   // Don't forget to pass an instance of the host class.
   *   queue->Push(&S::f, S());
   *
   *   // Pushing a const member function pointer.
   *   // This time, we wrap the instance of the host class inside a smart pointer.
   *   queue->Push(&S::f_const, std::unique_ptr<S>(new S()));
   * @endcode
   *
   * @warning DO NOT capture lvalue references in a lambda expression pushed into the queue
   * unless you can ensure that the function will be executed in the same scope where the input
   * lives. If not, such captures may be destroyed before the lambda is invoked by the queue.
   */
  template <class FT, class... ArgsT>
  SharedFuturePointer<InvokeResult<FT>> Push(FT&& f, ArgsT&&... args);

  /**
   * This method behaves the same way `Push` does, with the addition of a container of `futures`.
   * The function to be pushed will not be executed until the functions associated with the input
   * futures have terminated.
   *
   * The container of futures must have forward iterator (presence of a `begin()` and `end()` member
   * function).
   */
  template <class SharedFutureContainerT, class FT, class... ArgsT>
  SharedFuturePointer<InvokeResult<FT>> PushDependent(
    SharedFutureContainerT&& priorSharedFutures, FT&& f, ArgsT&&... args);

  /**
   * This method blocks the current thread until all the tasks associated with each shared future
   * inside `priorSharedFuture` has terminated.
   *
   * It is in general more efficient to call this function than to call `Wait` on each future
   * individually because
   * if any task associated with `priorSharedFuture` is allowed to run (i.e. it is not depending on
   * any other future) and is currently waiting in queue, this function will actually run it.
   *
   * The current thread is blocked at most once by this function.
   *
   * `SharedFutureContainerT` must have a forward iterator (presence of a `begin()` and `end()`
   * member function).
   */
  template <class SharedFutureContainerT>
  void Wait(SharedFutureContainerT&& priorSharedFuture);

  /**
   * Sets the number of threads. The running state of the queue is not impacted by this method.
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

private:
  ///@{
  /**
   * Invoker typedefs that hold the inserted functions and their parameters.
   *
   * `InvokerBase` is the base abstract type that helps us store the queue of functions to execute.
   * Each individual stored function is actually an instance of `Invoker` which inherits
   * `InvokerBase`. They share a pure virtual `operator()` that effectively calls the stored
   * function with the parameters provided.
   */
  struct InvokerBase;
  template <class FT, class... ArgsT>
  struct Invoker;
  struct InvokerImpl;
  ///@}

  using InvokerBasePointer = std::unique_ptr<InvokerBase>;
  template <class FT, class... ArgsT>
  using InvokerPointer = std::unique_ptr<Invoker<FT, ArgsT...>>;

  class ThreadWorker;

  friend class ThreadWorker;
  friend struct InvokerFutureSharedStateBase;
  template <class ReturnT, class DummyT>
  friend struct InvokerFutureSharedState;

  /**
   * Status that an invoker can be in.
   * This enum is used in InvokerFutureSharedStateBase.
   *
   * @note This is an exclusive status. The status should not combine those bits.
   */
  enum SharedStatus
  {
    /**
     * The shared state of this invoker might already have been shared with invokers it
     * depends on, but this invoker's status is still hanging. At this point we cannot tell if it
     * needs to be put in `InvokersOnHold` or just directly ran. An invoker seeing such a status in
     * a dependent invoker should ignore it.
     */
    CONSTRUCTING = 0x00,

    /**
     * The invoker is on hold, stored in `InvokersOnHold`.
     */
    ON_HOLD = 0x01,

    /**
     * The invoker is currently stored inside `InvokerQueue`. It is waiting to be picked up by a
     * thread.
     */
    ENQUEUED = 0x02,

    /**
     * The invoker is currently running its task.
     */
    RUNNING = 0x04,

    /**
     * The invoker has finished working and the returned value is available.
     */
    READY = 0x08
  };

  /**
   * This method terminates when all threads have finished. If `Destroying` is not true
   * then calling this method results in a deadlock.
   *
   * @param startId The thread id from which we synchronize the threads.
   */
  void Sync(int startId = 0);

  /**
   * Pops all the `nullptr` pointers at the front of `InvokerQueue` until either the queue is empty,
   * or the front is not `nullptr`.
   */
  void PopFrontNullptr();

  /**
   * We go over all the dependent future ids that have been added to the invoker we just invoked.
   * For each future, we retrieve the corresponding invoker that is stored inside InvokersOnHold,
   * and decrease the counter of the number of remaining invokers it depends on.
   * When this counter reaches zero, we can move the invoker from InvokersOnHold to InvokerQueues.
   */
  void SignalDependentSharedFutures(const InvokerBase* invoker);

  /**
   * This function takes an `invoker` associated with `futures`. If all futures from the input
   * `priorSharedFutures` are ready, then `invoker` is executed. Else, it is stored in an internal
   * container waiting to be awakened when its dependents futures have terminated.
   */
  template <class SharedFutureContainerT, class InvokerT, class SharedFutureT>
  void HandleDependentInvoker(
    SharedFutureContainerT&& priorSharedFutures, InvokerT&& invoker, SharedFutureT&& futures);

  /**
   * This function should always be used to invoke.
   * lock should be locked upon calling this function.
   */
  void Invoke(InvokerBasePointer&& invoker, std::unique_lock<std::mutex>& lock);

  /**
   * This will try to invoke the invoker owning a reference of `state`. The invoker will be ran if
   * and only if its status is `ENQUEUED`. If not, nothing happens.
   */
  bool TryInvoke(InvokerFutureSharedStateBase* state);

  /**
   * This function allocates an invoker and its bound future.
   */
  template <class FT, class... ArgsT>
  std::pair<InvokerPointer<FT, ArgsT...>, SharedFuturePointer<InvokeResult<FT>>>
  CreateInvokerAndSharedFuture(FT&& f, ArgsT&&... args);

  /**
   * Method to use when executing a control on the queue. Each control is run asynchronously, in the
   * order they were sent to the queue, by the queue itself.
   */
  template <class FT, class... ArgsT>
  void PushControl(FT&& f, ArgsT&&... args);

  /**
   * Returns true if any prior is not ready.
   */
  template <class SharedFutureContainerT>
  static bool MustWait(SharedFutureContainerT&& priorSharedFutures);

  /**
   * Queue of workers responsible for running the jobs that are inserted.
   */
  std::deque<InvokerBasePointer> InvokerQueue;

  /**
   * This is where we put invokers that are not ready to be run. This can happen in `PushDependent`
   * when an invoker associated with an input future is not done running yet.
   */
  std::unordered_map<vtkSharedFutureBase*, InvokerBasePointer> InvokersOnHold;

  /**
   * This mutex ensures that the queue can pop and push elements in a thread-safe manner.
   */
  std::mutex Mutex;

  /**
   * This mutex ensures that `InvokersOnHold` is accessed in a thread-safe manner.
   */
  std::mutex OnHoldMutex;

  /**
   * Mutex to use when interacting with `ControlFutures`.
   */
  std::mutex ControlMutex;

  /**
   * This mutex is used to synchronize destruction of this queue.
   * Any control should abort if the queue is being destroyed.
   */
  std::mutex DestroyMutex;

  /**
   * This mutex is used to protect access to `ThreadIdToIndex`.
   */
  std::mutex ThreadIdToIndexMutex;

  std::condition_variable ConditionVariable;

  /**
   * This atomic boolean is false until destruction. It is then used by the workers
   * so they know that they need to terminate when the queue is empty.
   */
  bool Destroying = false;

  /**
   * Number of allocated threads. Allocated threads are not necessarily running.
   */
  std::atomic_int NumberOfThreads;

  std::vector<std::thread> Threads;

  /**
   * Maps the thread id to its position inside `Threads`.
   *
   * This variable is used to swap threads when changing the number of threads. If we want to shrink
   * the number of threads and the thread executing the shrinkage is supposed to finish, we solve
   * the problem by swapping this thread id with the one of 0, who will finish in its place.
   */
  std::unordered_map<std::thread::id, std::shared_ptr<std::atomic_int>> ThreadIdToIndex;

  /**
   * Futures of controls that were passed to they queue. They allow to run controls in the same
   * order they were passed to the queue.
   */
  std::unordered_set<vtkSharedFutureBase*> ControlFutures;

  vtkThreadedCallbackQueue(const vtkThreadedCallbackQueue&) = delete;
  void operator=(const vtkThreadedCallbackQueue&) = delete;
};

VTK_ABI_NAMESPACE_END

#include "vtkThreadedCallbackQueue.txx"

#endif
#endif
// VTK-HeaderTest-Exclude: vtkThreadedCallbackQueue.h
