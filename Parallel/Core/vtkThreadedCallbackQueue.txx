/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedCallbackQueue.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkObjectFactory.h"

#include <chrono>
#include <tuple>
#include <type_traits>

VTK_ABI_NAMESPACE_BEGIN

//=============================================================================
struct vtkThreadedCallbackQueue::InvokerTokenSharedState
{
  InvokerTokenSharedState()
    : NumberOfPriorTokensRemaining(0)
  {
  }

  /**
   * Number of tokens that need to terminate before we can run.
   */
  std::atomic_int NumberOfPriorTokensRemaining;

  /**
   * List of tokens which are depending on us. This is filled by them as they get pushed if we are
   * not done with our task.
   */
  std::vector<TokenBasePointer> DependentTokens;

  /**
   * Constructing is true until we have filled the DependentTokens of the tokens we depend on, and
   * until NumberOfPriorTokensRemaining is not final.
   */
  bool Constructing = true;

  std::mutex Mutex;
};

//=============================================================================
struct vtkThreadedCallbackQueue::InvokerImpl
{
  /**
   * Substitute for std::integer_sequence which is C++14
   */
  template <std::size_t... Is>
  struct IntegerSequence;
  template <std::size_t N, std::size_t... Is>
  struct MakeIntegerSequence;

  /**
   * This function is used to discriminate whether you can dereference the template parameter T.
   * It uses SNFINAE and jumps to the std::false_type version if it fails dereferencing T.
   */
  template <class T>
  static decltype(*std::declval<T&>(), std::true_type{}) CanBeDereferenced(std::nullptr_t);
  template <class>
  static std::false_type CanBeDereferenced(...);

  template <class T, class CanBeDereferencedT = decltype(CanBeDereferenced<T>(nullptr))>
  struct DereferenceImpl;

  /**
   * Convenient typedef that use Signature to convert the parameters of function of type FT
   * to a std::tuple.
   */
  template <class FT, std::size_t N = Signature<typename std::decay<FT>::type>::ArgsSize>
  using ArgsTuple = typename Signature<typename std::decay<FT>::type>::ArgsTuple;

  /**
   * Convenient typedef that, given a function of type FT and an index I, returns the type of the
   * Ith parameter of the function.
   */
  template <class FT, std::size_t I>
  using ArgType = typename std::tuple_element<I, ArgsTuple<FT>>::type;

  /**
   * This helper function returns a tuple of cherry-picked types from the argument types from
   * the input function or the argument types of the provided input following this criterion:
   * * If the input function expects an lvalue reference, store it in its decayed type.
   * * Otherwise, store it as is (decayed input type). The conversion to the function argument
   *   type will be done upon invoking.
   *
   * This specific casting allows us to permit calling the constructor for rvalue reference inputs
   * when the type differs from the one provided by the user (take a const char* input when the
   * function expects a std::string for instance).
   * We want to keep the input type in all other circumstances in case the user inputs smart
   * pointers and the function only expects a raw pointer. If we casted it to the function argument
   * type, we would not own a reference of the smart pointer.
   *
   * FunctionArgsTupleT is a tuple of the function native argument types (extracted from its
   * signature), and InputArgsTupleT is a tuple of the types provided by the user as input
   * parameters.
   */
  template <class FunctionArgsTupleT, class InputArgsTupleT, std::size_t... Is>
  static std::tuple<typename std::conditional<
    std::is_lvalue_reference<typename std::tuple_element<Is, FunctionArgsTupleT>::type>::value,
    typename std::decay<typename std::tuple_element<Is, FunctionArgsTupleT>::type>::type,
    typename std::decay<typename std::tuple_element<Is, InputArgsTupleT>::type>::type>::type...>
  GetStaticCastArgsTuple(IntegerSequence<Is...>);

  /**
   * Convenient typedef to create a tuple of types allowing to call the constructor of the function
   * argument type when relevant, or hold a copy of the input parameters provided by the user
   * instead.
   */
  template <class FunctionArgsTupleT, class... InputArgsT>
  using StaticCastArgsTuple =
    decltype(GetStaticCastArgsTuple<FunctionArgsTupleT, std::tuple<InputArgsT...>>(
      MakeIntegerSequence<sizeof...(InputArgsT)>()));

  /**
   * This holds the attributes of a function.
   * There are 2 implementations: one for member function pointers, and one for all the others
   * (functors, lambdas, function pointers)
   */
  template <bool IsMemberFunctionPointer, class... ArgsT>
  class InvokerHandle;

  /**
   * Actually invokes the function and sets its future. An specific implementation is needed for
   * void return types.
   */
  template <class ReturnT>
  struct InvokerHelper
  {
    template <class InvokerT>
    static void Invoke(InvokerT&& invoker, std::promise<ReturnT>& p)
    {
      p.set_value(invoker());
    }
  };
};

//=============================================================================
template <>
struct vtkThreadedCallbackQueue::InvokerImpl::InvokerHelper<void>
{
  template <class InvokerT>
  static void Invoke(InvokerT&& invoker, std::promise<void>& p)
  {
    invoker();
    p.set_value();
  }
};

//=============================================================================
// For lamdas or std::function
template <class ReturnT, class... ArgsT>
struct vtkThreadedCallbackQueue::Signature<ReturnT(ArgsT...)>
{
  using ArgsTuple = std::tuple<ArgsT...>;
  using InvokeResult = ReturnT;
  static constexpr std::size_t ArgsSize = sizeof...(ArgsT);
};

//=============================================================================
// For methods inside a class ClassT
template <class ClassT, class ReturnT, class... ArgsT>
struct vtkThreadedCallbackQueue::Signature<ReturnT (ClassT::*)(ArgsT...)>
{
  using ArgsTuple = std::tuple<ArgsT...>;
  using InvokeResult = ReturnT;
  static constexpr std::size_t ArgsSize = sizeof...(ArgsT);
};

//=============================================================================
// For const methods inside a class ClassT
template <class ClassT, class ReturnT, class... ArgsT>
struct vtkThreadedCallbackQueue::Signature<ReturnT (ClassT::*)(ArgsT...) const>
{
  using ArgsTuple = std::tuple<ArgsT...>;
  using InvokeResult = ReturnT;
  static constexpr std::size_t ArgsSize = sizeof...(ArgsT);
};

//=============================================================================
// For function pointers
template <class ReturnT, class... ArgsT>
struct vtkThreadedCallbackQueue::Signature<ReturnT (*)(ArgsT...)>
{
  using ArgsTuple = std::tuple<ArgsT...>;
  using InvokeResult = ReturnT;
  static constexpr std::size_t ArgsSize = sizeof...(ArgsT);
};

//=============================================================================
// For functors
template <class FT>
struct vtkThreadedCallbackQueue::Signature
  : vtkThreadedCallbackQueue::Signature<decltype(&FT::operator())>
{
};

//=============================================================================
template <std::size_t... Is>
struct vtkThreadedCallbackQueue::InvokerImpl::IntegerSequence
{
};

//=============================================================================
template <std::size_t N, std::size_t... Is>
struct vtkThreadedCallbackQueue::InvokerImpl::MakeIntegerSequence
  : vtkThreadedCallbackQueue::InvokerImpl::MakeIntegerSequence<N - 1, N - 1, Is...>
{
};

//=============================================================================
template <std::size_t... Is>
struct vtkThreadedCallbackQueue::InvokerImpl::MakeIntegerSequence<0, Is...>
  : vtkThreadedCallbackQueue::InvokerImpl::IntegerSequence<Is...>
{
};

//=============================================================================
template <class T>
struct vtkThreadedCallbackQueue::InvokerImpl::DereferenceImpl<T,
  std::true_type /* CanBeDereferencedT */>
{
  using Type = decltype(*std::declval<T>());
  static Type& Get(T& instance) { return *instance; }
};

//=============================================================================
template <class T>
struct vtkThreadedCallbackQueue::InvokerImpl::DereferenceImpl<T,
  std::false_type /* CanBeDereferencedT */>
{
  using Type = T;
  static Type& Get(T& instance) { return instance; }
};

//=============================================================================
template <class T>
struct vtkThreadedCallbackQueue::Dereference<T, std::nullptr_t>
{
  using Type = typename InvokerImpl::DereferenceImpl<T>::Type;
};

//=============================================================================
template <class FT, class ObjectT, class... ArgsT>
class vtkThreadedCallbackQueue::InvokerImpl::InvokerHandle<true /* IsMemberFunctionPointer */, FT,
  ObjectT, ArgsT...>
{
public:
  template <class FTT, class ObjectTT, class... ArgsTT>
  InvokerHandle(FTT&& f, ObjectTT&& instance, ArgsTT&&... args)
    : Function(std::forward<FTT>(f))
    , Instance(std::forward<ObjectTT>(instance))
    , Args(std::forward<ArgsTT>(args)...)
  {
  }

  InvokeResult<FT> operator()() { this->Invoke(MakeIntegerSequence<sizeof...(ArgsT)>()); }

private:
  template <std::size_t... Is>
  InvokeResult<FT> Invoke(IntegerSequence<Is...>)
  {
    // If the input object is wrapped inside a pointer (could be shared_ptr, vtkSmartPointer),
    // we need to dereference the object before invoking it.
    auto& deref = DereferenceImpl<ObjectT>::Get(this->Instance);

    // The static_cast to ArgType forces casts to the correct types of the function.
    // There are conflicts with rvalue references not being able to be converted to lvalue
    // references if this static_cast is not performed
    return (deref.*Function)(static_cast<ArgType<FT, Is>>(std::get<Is>(this->Args))...);
  }

  FT Function;

  // We DO NOT want to hold lvalue references! They could be destroyed before we execute them.
  // This forces to call the copy constructor on lvalue references inputs.
  typename std::decay<ObjectT>::type Instance;

  // We want to hold an instance of the arguments in the type expected by the function rather than
  // the types provided by the user when the function expects a lvalue reference.
  // This way, if there is a conversion to be done, it can be done
  // in the constructor of each type.
  //
  // Example: The user provides a string as "example", but the function expects a std::string&.
  // We can directly store this argument as a std::string and allow to pass it to the function as a
  // std::string.
  StaticCastArgsTuple<ArgsTuple<FT>, ArgsT...> Args;
};

//=============================================================================
template <class FT, class... ArgsT>
class vtkThreadedCallbackQueue::InvokerImpl::InvokerHandle<false /* IsMemberFunctionPointer */, FT,
  ArgsT...>
{
public:
  template <class FTT, class... ArgsTT>
  InvokerHandle(FTT&& f, ArgsTT&&... args)
    : Function(std::forward<FTT>(f))
    , Args(std::forward<ArgsTT>(args)...)
  {
  }

  InvokeResult<FT> operator()() { return this->Invoke(MakeIntegerSequence<sizeof...(ArgsT)>()); }

private:
  template <std::size_t... Is>
  InvokeResult<FT> Invoke(IntegerSequence<Is...>)
  {
    // If the input is a functor and is wrapped inside a pointer (could be shared_ptr),
    // we need to dereference the functor before invoking it.
    auto& f = DereferenceImpl<FT>::Get(this->Function);

    // The static_cast to ArgType forces casts to the correct types of the function.
    // There are conflicts with rvalue references not being able to be converted to lvalue
    // references if this static_cast is not performed
    return f(static_cast<ArgType<decltype(f), Is>>(std::get<Is>(this->Args))...);
  }

  // We DO NOT want to hold lvalue references! They could be destroyed before we execute them.
  // This forces to call the copy constructor on lvalue references inputs.
  typename std::decay<FT>::type Function;

  // We want to hold an instance of the arguments in the type expected by the function rather than
  // the types provided by the user when the function expects a lvalue reference.
  // This way, if there is a conversion to be done, it can be done
  // in the constructor of each type.
  //
  // Example: The user provides a string as "example", but the function expects a std::string&.
  // We can directly store this argument as a std::string and allow to pass it to the function as a
  // std::string.
  StaticCastArgsTuple<ArgsTuple<typename Dereference<FT>::Type>, ArgsT...> Args;
};

//=============================================================================
struct vtkThreadedCallbackQueue::InvokerBase
{
  InvokerBase(std::shared_ptr<InvokerTokenSharedState>& sharedState)
    : SharedState(sharedState)
  {
  }

  virtual ~InvokerBase() = default;
  virtual void operator()() = 0;

  std::shared_ptr<InvokerTokenSharedState> SharedState;
};

//=============================================================================
template <class FT, class... ArgsT>
struct vtkThreadedCallbackQueue::Invoker : public vtkThreadedCallbackQueue::InvokerBase
{
  template <class... ArgsTT>
  Invoker(std::shared_ptr<InvokerTokenSharedState>& sharedState, ArgsTT&&... args)
    : InvokerBase(sharedState)
    , Impl(std::forward<ArgsTT>(args)...)
  {
  }

  void operator()() override
  {
    InvokerImpl::InvokerHelper<InvokeResult<FT>>::Invoke(this->Impl, this->Promise);
  }

  std::shared_future<InvokeResult<FT>> GetFuture()
  {
    return std::shared_future<InvokeResult<FT>>(this->Future);
  }

  InvokerImpl::InvokerHandle<std::is_member_function_pointer<FT>::value, FT, ArgsT...> Impl;
  std::promise<InvokeResult<FT>> Promise;
  std::shared_future<InvokeResult<FT>> Future = Promise.get_future();
};

//-----------------------------------------------------------------------------
template <class ReturnT>
vtkThreadedCallbackQueue::vtkCallbackToken<ReturnT>*
vtkThreadedCallbackQueue::vtkCallbackToken<ReturnT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkThreadedCallbackQueue::vtkCallbackToken<ReturnT>);
}

//-----------------------------------------------------------------------------
template <class ReturnT>
vtkThreadedCallbackQueue::TokenBasePointer
vtkThreadedCallbackQueue::vtkCallbackToken<ReturnT>::Clone() const
{
  vtkCallbackToken<ReturnT>* result = vtkCallbackToken<ReturnT>::New();
  result->Future = this->Future;
  result->SharedState = this->SharedState;
  return TokenBasePointer::Take(result);
}

//-----------------------------------------------------------------------------
template <class ReturnT>
void vtkThreadedCallbackQueue::vtkCallbackToken<ReturnT>::Wait() const
{
  this->Future.wait();
}

//-----------------------------------------------------------------------------
template <class ReturnT>
bool vtkThreadedCallbackQueue::vtkCallbackToken<ReturnT>::IsReady() const
{
  return this->Future.wait_for(std::chrono::microseconds(0)) == std::future_status::ready;
}

//-----------------------------------------------------------------------------
template <class FT, class... ArgsT>
std::pair<vtkThreadedCallbackQueue::InvokerPointer<FT, ArgsT...>,
  vtkThreadedCallbackQueue::TokenPointer<vtkThreadedCallbackQueue::InvokeResult<FT>>>
vtkThreadedCallbackQueue::CreateInvokerAndToken(FT&& f, ArgsT&&... args)
{
  auto token = TokenPointer<InvokeResult<FT>>::New();
  token->SharedState = std::shared_ptr<InvokerTokenSharedState>(new InvokerTokenSharedState());
  auto invoker = InvokerPointer<FT, ArgsT...>(new Invoker<FT, ArgsT...>(
    token->SharedState, std::forward<FT>(f), std::forward<ArgsT>(args)...));
  token->Future = invoker->GetFuture();

  return std::make_pair(std::move(invoker), std::move(token));
}

//-----------------------------------------------------------------------------
template <class TokenContainerT, class InvokerT, class TokenT>
void vtkThreadedCallbackQueue::HandleDependentInvoker(
  TokenContainerT&& priorTokens, InvokerT&& invoker, TokenT&& token)
{
  // We look at all the dependent tokens. Each time we find one, we notify the corresponding
  // invoker that we are waiting on it through its SharedState.
  // We count the number of dependents that are not done yet, and we put this counter bundled
  // with the new invoker inside InvokersOnHold.
  // When the signaled invokers terminate, the counter will be decreased, and when it reaches
  // zero, this invoker will be ready to run.
  if (!priorTokens.empty())
  {
    for (const auto& prior : priorTokens)
    {
      // For each prior, we create a clone so there is no race condition on the same shared_ptr.
      TokenBasePointer clone = prior->Clone();

      // TODO easy continue if clone is ready.
      // We need to lock the clone (so we block the invoker side).
      // This way, we can make sure that if the invoker is still running, we notify it that we
      // depend on it before it checks its dependents in SignalDependentTokens
      std::unique_lock<std::mutex> lock(clone->SharedState->Mutex);
      if (!clone->IsReady())
      {
        // We notify the invoker we depend on by adding ourselves in DependentTokens.
        clone->SharedState->DependentTokens.emplace_back(token);

        // This does not need to be locked because the shared state of this token is not done
        // constructing yet, so the invoker in SignalDependentTokens will never try do anything with
        // it. And it is okay, because at the end of the day, we increment, the invoker decrements,
        // and if we end up with 0 remaining prior tokens, we execute the invoker anyway, so the
        // invoker side has nothing to do.
        lock.unlock();
        ++token->SharedState->NumberOfPriorTokensRemaining;
      }
    }
  }
  // We notify every invokers we depend on that our SharedState is done constructing. This means
  // that we inserted ourselves in InvokersOnHold if necessary and that if the invoker finds no
  // remaining prior tokens in this dependent, it can move the invoker associated with us to the
  // runnin queue.
  std::unique_lock<std::mutex> lock(token->SharedState->Mutex);
  token->SharedState->Constructing = false;
  if (token->SharedState->NumberOfPriorTokensRemaining)
  {
    std::lock_guard<std::mutex> onHoldLock(this->OnHoldMutex);
    this->InvokersOnHold.emplace(token, std::move(invoker));
  }
  else
  {
    // We can unlock the token lock because we're done touching sensitive data. There's no prior
    // token we're waiting for, so we can just run the invoker and signal our potential dependents.
    lock.unlock();
    (*invoker)();
    this->SignalDependentTokens(invoker.get());
  }
}

//-----------------------------------------------------------------------------
template <class TokenContainerT, class FT, class... ArgsT>
vtkThreadedCallbackQueue::TokenPointer<vtkThreadedCallbackQueue::InvokeResult<FT>>
vtkThreadedCallbackQueue::PushDependent(TokenContainerT&& priorTokens, FT&& f, ArgsT&&... args)
{
  using InvokerPointerType = InvokerPointer<FT, ArgsT...>;
  using TokenPointerType = TokenPointer<InvokeResult<FT>>;

  auto pair = this->CreateInvokerAndToken(std::forward<FT>(f), std::forward<ArgsT>(args)...);

  this->Push(&vtkThreadedCallbackQueue::HandleDependentInvoker<TokenContainerT, InvokerPointerType,
               TokenPointerType>,
    this, std::forward<TokenContainerT>(priorTokens), std::move(pair.first), pair.second);

  return pair.second;
}

//-----------------------------------------------------------------------------
template <class FT, class... ArgsT>
vtkThreadedCallbackQueue::TokenPointer<vtkThreadedCallbackQueue::InvokeResult<FT>>
vtkThreadedCallbackQueue::Push(FT&& f, ArgsT&&... args)
{
  auto pair = this->CreateInvokerAndToken(std::forward<FT>(f), std::forward<ArgsT>(args)...);

  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    this->InvokerQueue.emplace_back(std::move(pair.first));
    this->Empty = false;
  }

  this->ConditionVariable.notify_one();

  return pair.second;
}

VTK_ABI_NAMESPACE_END
