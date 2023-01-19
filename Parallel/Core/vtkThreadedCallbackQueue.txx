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

#include <functional>
#include <tuple>
#include <type_traits>

VTK_ABI_NAMESPACE_BEGIN

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
  virtual ~InvokerBase() = default;
  virtual void operator()() = 0;
};

//=============================================================================
template <class FT, class... ArgsT>
class vtkThreadedCallbackQueue::Invoker : public vtkThreadedCallbackQueue::InvokerBase
{
public:
  template <class... ArgsTT>
  Invoker(ArgsTT&&... args)
    : Impl(std::forward<ArgsTT>(args)...)
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

private:
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
vtkSmartPointer<vtkThreadedCallbackQueue::vtkCallbackTokenBase>
vtkThreadedCallbackQueue::vtkCallbackToken<ReturnT>::Clone() const
{
  vtkCallbackToken<ReturnT>* result = vtkCallbackToken<ReturnT>::New();
  result->Future = this->Future;
  return vtkSmartPointer<vtkCallbackTokenBase>::Take(result);
}

//-----------------------------------------------------------------------------
template <class ReturnT>
void vtkThreadedCallbackQueue::vtkCallbackToken<ReturnT>::Wait() const
{
  this->Future.wait();
}

//-----------------------------------------------------------------------------
template <class TokenContainerT, class FT, class... ArgsT>
vtkSmartPointer<
  vtkThreadedCallbackQueue::vtkCallbackToken<vtkThreadedCallbackQueue::InvokeResult<FT>>>
vtkThreadedCallbackQueue::PushDependent(TokenContainerT&& tokens, FT&& f, ArgsT&&... args)
{
  using InvokerType = Invoker<FT, ArgsT...>;
  using TokenType = vtkCallbackToken<InvokeResult<FT>>;

  auto invoker = std::unique_ptr<InvokerType>(
    new InvokerType(std::forward<FT>(f), std::forward<ArgsT>(args)...));
  auto newToken = vtkSmartPointer<TokenType>::New();
  newToken->Future = invoker->GetFuture();

  // TODO
  // Be smarter about this. This is a waste to make threads wait for other threads.
  // A better design would involve storing the invoker, and let know the invokers on which it
  // depends on that this invoker is waiting. When the invokers terminate, they could tell the
  // waiting invokers that they have terminated. When an invoker doesn't wait on anyone, we could
  // push it back into the waiting queue.
  //
  // If we associate to each invoker an ID, we can store the waiting invokers inside a hash map, and
  // probably store the number of invokers it is waiting on.
  // The communication between the waiting invokers and the invokers on which they depend on
  // could be done through
  // a shared variable stored inside the tokens. We could put a container of waiting ids inside it,
  // and the invoker could go through them when it is done and decrease the counter of the waiting
  // invoker. When the waiting invoker reaches zero, push it back into the queue.
  //
  // We could also refine the pushing back into the queue in such a way that it is not placed before
  // invokers of greater ID by having a third invoker container in the form of a priority queue that
  // could be popped when the front of the invoker queue has an id higher than the top of the
  // priority queue. The invoker could be then pushed to the front (which means we need
  // InvokerQueue to be a std::deque).
  this->Push(
    [](TokenContainerT&& _tokens, std::unique_ptr<InvokerType>&& _invoker) {
      for (const auto& token : _tokens)
      {
        token->Clone()->Wait();
      }
      (*_invoker)();
    },
    std::forward<TokenContainerT>(tokens), std::move(invoker));

  return newToken;
}

//-----------------------------------------------------------------------------
template <class FT, class... ArgsT>
vtkSmartPointer<
  vtkThreadedCallbackQueue::vtkCallbackToken<vtkThreadedCallbackQueue::InvokeResult<FT>>>
vtkThreadedCallbackQueue::Push(FT&& f, ArgsT&&... args)
{
  using InvokerType = Invoker<FT, ArgsT...>;
  using TokenType = vtkCallbackToken<InvokeResult<FT>>;

  auto invoker = std::unique_ptr<InvokerType>(
    new InvokerType(std::forward<FT>(f), std::forward<ArgsT>(args)...));
  auto token = vtkSmartPointer<TokenType>::New();
  token->Future = invoker->GetFuture();

  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    this->InvokerQueue.emplace(std::move(invoker));
    this->Empty = false;
  }

  this->ConditionVariable.notify_one();

  return token;
}

VTK_ABI_NAMESPACE_END
