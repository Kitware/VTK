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
 * All public methods of this class are thread safe.
 *
 * @warning All pushed tasks MUST terminate! If not, one could end up with a deadlock when using
 * `vtkCallbackTokenBase::Wait()` on the token associated with this task.
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
#include <future>             // For future
#include <memory>             // For unique_ptr
#include <mutex>              // For mutex
#include <thread>             // For thread
#include <unordered_map>      // For unordered_map
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

  struct InvokerTokenSharedState;

  /**
   * A `vtkCallbackTokenBase` is an object returned by the methods `Push` and `PushDependent`.
   * It provides a few functionalities to allow one to synchronize tasks.
   * This token is associated with the task that was pushed.
   */
  class vtkCallbackTokenBase : public vtkObjectBase
  {
  public:
    vtkBaseTypeMacro(vtkCallbackTokenBase, vtkObjectBase);

    vtkCallbackTokenBase() = default;

    /**
     * Blocks current thread until the task associated with this token has terminated.
     */
    virtual void Wait() const = 0;

    /**
     * Returns a copy of this instance.
     */
    virtual vtkSmartPointer<vtkCallbackTokenBase> Clone() const = 0;

    virtual bool IsReady() const = 0;

    friend class vtkThreadedCallbackQueue;

  private:
    std::shared_ptr<InvokerTokenSharedState> SharedState;

    vtkCallbackTokenBase(const vtkCallbackTokenBase& other) = delete;
    void operator=(const vtkCallbackTokenBase& other) = delete;
  };

  /**
   * A `vtkCallbackToken` is an object returned by the methods `Push` and `PushDependent`.
   * In addition to implementing the pure virtual methods of `vtkCallbackTokenBase`,
   * this class gives access to a `std::shared_future`.
   */
  template <class ReturnT>
  class vtkCallbackToken : public vtkCallbackTokenBase
  {
  public:
    static vtkCallbackToken<ReturnT>* New();
    vtkTypeMacro(vtkCallbackToken<ReturnT>, vtkCallbackTokenBase);

    vtkCallbackToken() = default;

    vtkSmartPointer<vtkCallbackTokenBase> Clone() const override;
    void Wait() const override;
    bool IsReady() const override;

    /**
     * Future of the task associated with this token.
     */
    std::shared_future<ReturnT> Future;

  private:
    vtkCallbackToken(const vtkCallbackToken<ReturnT>& other) = delete;
    void operator=(const vtkCallbackToken<ReturnT>& other) = delete;
  };

  using TokenBasePointer = vtkSmartPointer<vtkCallbackTokenBase>;
  template <class ReturnT>
  using TokenPointer = vtkSmartPointer<vtkCallbackToken<ReturnT>>;

  /**
   * Pushes a function f to be passed args... as arguments.
   * f will be called as soon as a running thread has the occasion to do so, in a FIFO fashion.
   * This method returns a `Token`, which is an object
   * allowing to synchronize the code at will through the use of a `std::shared_future`.
   * This method is thread-safe.
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
  TokenPointer<InvokeResult<FT>> Push(FT&& f, ArgsT&&... args);

  /**
   * This method behaves the same way `Push` does, with the addition of a container of `tokens`.
   * The function to be pushed will not be executed until the functions associated with the input
   * tokens have terminated.
   *
   * The container of tokens must have forward iterator (presence of a `begin()` and `end()` member
   * function).
   */
  template <class TokenContainerT, class FT, class... ArgsT>
  TokenPointer<InvokeResult<FT>> PushDependent(
    TokenContainerT&& priorTokens, FT&& f, ArgsT&&... args);

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

  /**
   * Starts the threads as soon as they are done with their current tasks.
   *
   * This method is executed by the `Controller` on a different thread, so this method may terminate
   * before the threads are spawned. Nevertheless, this method is thread-safe. Other calls to
   * `Start()` will be queued by the `Controller`, which executes all received command serially in
   * the background.
   */
  void Start();

  /**
   * This method terminates when all threads have finished. If `Destroying` is not true
   * then calling this method results in a deadlock.
   *
   * @param startId The thread id from which we synchronize the threads.
   */
  void Sync(int startId = 0);

  /**
   * We go over all the dependent token ids that have been added to the invoker we just invoked.
   * For each token, we retrieve the corresponding invoker that is stored inside InvokersOnHold,
   * and decrease the counter of the number of remaining invokers it depends on.
   * When this counter reaches zero, we can move the invoker from InvokersOnHold to InvokerQueues.
   */
  void SignalDependentTokens(const InvokerBase* invoker);

  /**
   * This function takes an `invoker` associated with `tokens`. If all tokens from the input
   * `priorTokens` are ready, then `invoker` is executed. Else, it is stored in an internal
   * container waiting to be awakened when its dependents tokens have terminated.
   */
  template <class TokenContainerT, class InvokerT, class TokenT>
  void HandleDependentInvoker(TokenContainerT&& priorTokens, InvokerT&& invoker, TokenT&& tokens);

  /**
   * This function allocates an invoker and its bound token.
   */
  template <class FT, class... ArgsT>
  std::pair<InvokerPointer<FT, ArgsT...>, TokenPointer<InvokeResult<FT>>> CreateInvokerAndToken(
    FT&& f, ArgsT&&... args);

  /**
   * Queue of workers responsible for running the jobs that are inserted.
   */
  std::deque<InvokerBasePointer> InvokerQueue;

  /**
   * This is where we put invokers that are not ready to be run. This can happen in `PushDependent`
   * when an invoker associated with an input token is not done running yet.
   */
  std::unordered_map<vtkCallbackTokenBase*, InvokerBasePointer> InvokersOnHold;

  /**
   * This mutex ensures that the queue can pop and push elements in a thread-safe manner.
   */
  std::mutex Mutex;

  /**
   * This mutex ensures that `InvokersOnHold` is accessed in a thread-safe manner.
   */
  std::mutex OnHoldMutex;

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
   * Number of allocated threads. Allocated threads are not necessarily running.
   */
  std::atomic_int NumberOfThreads;

  std::vector<std::thread> Threads;

  /**
   * The controller is responsible for taking care of the calls to `Start()`, and
   * `SetNumberOfThreads(int)`. It queues those commands and serially executes them on a separate
   * thread. This allows those methods to not be blocking and run asynchronously.
   */
  vtkSmartPointer<vtkThreadedCallbackQueue> Controller;

  vtkThreadedCallbackQueue(const vtkThreadedCallbackQueue&) = delete;
  void operator=(const vtkThreadedCallbackQueue&) = delete;

  static vtkThreadedCallbackQueue* New(vtkThreadedCallbackQueue* controller);

  /**
   * Constructor setting internal `Controller` to the provided controller.
   */
  vtkThreadedCallbackQueue(vtkSmartPointer<vtkThreadedCallbackQueue>&& controller);
};

VTK_ABI_NAMESPACE_END

#include "vtkThreadedCallbackQueue.txx"

#endif
#endif
// VTK-HeaderTest-Exclude: vtkThreadedCallbackQueue.h
