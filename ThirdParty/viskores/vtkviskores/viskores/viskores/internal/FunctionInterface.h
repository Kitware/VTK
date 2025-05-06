//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_internal_FunctionInterface_h
#define viskores_internal_FunctionInterface_h

#include <viskores/Deprecated.h>
#include <viskores/Types.h>

#include <viskores/internal/FunctionInterfaceDetailPre.h>
#include <viskores/internal/IndexTag.h>

#include <utility>

namespace viskores
{
namespace internal
{

namespace detail
{

template <typename OriginalSignature, typename Transform>
struct FunctionInterfaceStaticTransformType;


} // namespace detail

/// \brief Holds parameters and result of a function.
///
/// To make Viskores easier for the end user developer, the
/// \c Invoke method of dispatchers takes an arbitrary amount of
/// arguments that get transformed and swizzled into arguments and return value
/// for a worklet operator. In between these two invocations a complicated
/// series of transformations and operations can occur.
///
/// Supporting arbitrary function and template arguments is difficult and
/// really requires separate implementations for pre-C++11 and C++11 versions of
/// compilers. Thus, variadic template arguments are, at this point in time,
/// something to be avoided when possible. The intention of \c
/// FunctionInterface is to collect most of the variadic template code into one
/// place. The \c FunctionInterface template class takes a function signature,
/// which can have a variable number of arguments. The \c FunctionInterface
/// will hold in its state a copy of all input parameters (regardless of number
/// or type) and the return value if it exists (i.e. non-nullptr) and the function
/// has been invoked. This means that all arguments can be passed around in a
/// single object so that objects and functions dealing with these variadic
/// parameters can be templated on a single type (the type of \c
/// FunctionInterface).
///
/// Note that the indexing of the parameters in a \c FunctionInterface starts
/// at 1. You can think of the return value being the parameter at index 0,
/// even if there is no return value. Although this is uncommon in C++, it
/// matches better the parameter indexing for other classes that deal with
/// function signatures.
///
/// The \c FunctionInterface contains several ways to invoke a functor whose
/// parameters match those of the function interface. This allows you to
/// complete the transition of calling an arbitrary function (like a worklet).
///
/// The following is a rundown of how a \c FunctionInterface is created and
/// used. See the independent documentation for more details.
///
/// Use the \c make_FunctionInterface function to create a \c FunctionInterface
/// and initialize the state of all the parameters. \c make_FunctionInterface
/// takes a variable number of arguments, one for each parameter. Since the
/// return type is not specified as an argument, you must always specify it as
/// a template parameter.
///
/// \code{.cpp}
/// viskores::internal::FunctionInterface<void(viskores::IdComponent,double,char)> functionInterface =
///     viskores::internal::make_FunctionInterface<void>(1, 2.5, 'a');
/// \endcode
///
/// The number of parameters can be retrieved either with the constant field
/// \c ARITY or with the \c GetArity method.
///
/// \code{.cpp}
/// functionInterface.GetArity();
/// \endcode
///
/// You can get a particular parameter using the templated function \c
/// ParameterGet. The template parameter is the index of the parameter
/// (starting at 1).
///
/// Finally, there is a way to replace all of the parameters at
/// once. The \c StaticTransform methods take a transform functor that modifies
/// each of the parameters. See the documentation for this method for
/// details on how it is used.
///
template <typename FunctionSignature>
class FunctionInterface
{
  template <typename OtherSignature>
  friend class FunctionInterface;

public:
  using Signature = FunctionSignature;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  FunctionInterface()
    : Parameters()
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  explicit FunctionInterface(const detail::ParameterContainer<FunctionSignature>& p)
    : Parameters(p)
  {
  }

  // the number of parameters as an integral constant
  using SigInfo = detail::FunctionSigInfo<FunctionSignature>;
  using ComponentSig = typename SigInfo::Components;
  using ParameterSig = typename SigInfo::Parameters;

  template <viskores::IdComponent ParameterIndex>
  struct ParameterType
  {
    using type = typename detail::AtType<ParameterIndex, FunctionSignature>::type;
  };

  /// The number of parameters in this \c Function Interface.
  ///
  static constexpr viskores::IdComponent ARITY = SigInfo::Arity;

  /// Returns the number of parameters held in this \c FunctionInterface. The
  /// return value is the same as \c ARITY.
  ///
  VISKORES_EXEC_CONT
  viskores::IdComponent GetArity() const { return ARITY; }

  template <typename Transform>
  struct StaticTransformType
  {
    using type = FunctionInterface<
      typename detail::FunctionInterfaceStaticTransformType<FunctionSignature, Transform>::type>;
  };

  /// \brief Transforms the \c FunctionInterface based on compile-time
  /// information.
  ///
  /// The \c StaticTransform methods transform all the parameters of this \c
  /// FunctionInterface to different types and values based on compile-time
  /// information. It operates by accepting a functor that two arguments. The
  /// first argument is the parameter to transform and the second argument is
  /// an \c IndexTag specifying the index of the parameter (which can be
  /// ignored in many cases). The functor's return value is the transformed
  /// value. The functor must also contain a templated struct name ReturnType
  /// with an internal type named \c type that defines the return type of the
  /// transform for a given input type and parameter index.
  ///
  /// The transformation is only applied to the parameters of the function. The
  /// return argument is unaffected.
  ///
  /// The return type can be determined with the \c StaticTransformType
  /// template.
  ///
  /// Here is an example of a transformation that converts a \c
  /// FunctionInterface to another \c FunctionInterface containing pointers to
  /// all of the parameters.
  ///
  /// \code
  /// struct MyTransformFunctor {
  ///   template<typename T, viskores::IdComponent Index>
  ///   struct ReturnType {
  ///     typedef const T *type;
  ///   };
  ///
  ///   template<typename T, viskores::IdComponent Index>
  ///   VISKORES_CONT
  ///   const T *operator()(const T &x, viskores::internal::IndexTag<Index>) const {
  ///     return &x;
  ///   }
  /// };
  ///
  /// template<typename FunctionSignature>
  /// typename viskores::internal::FunctionInterface<FunctionSignature>::template StaticTransformType<MyTransformFunctor>::type
  /// ImportantStuff(const viskores::internal::FunctionInterface<FunctionSignature> &funcInterface)
  /// {
  ///   return funcInterface.StaticTransformCont(MyTransformFunctor());
  /// }
  /// \endcode
  ///
  template <typename Transform>
  VISKORES_CONT typename StaticTransformType<Transform>::type StaticTransformCont(
    const Transform& transform)
  {
    using FuncIface = typename StaticTransformType<Transform>::type;
    using PC = detail::ParameterContainer<typename FuncIface::Signature>;
    return FuncIface{ detail::DoStaticTransformCont<PC>(transform, this->Parameters) };
  }

  detail::ParameterContainer<FunctionSignature> Parameters;
};

/// Gets the value for the parameter of the given index. Parameters are
/// indexed starting at 1. To use this method you have to specify a static,
/// compile time index.
///
/// \code{.cpp}
/// template<FunctionSignature>
/// void Foo(const viskores::internal::FunctionInterface<FunctionSignature> &fInterface)
/// {
///   bar( ParameterGet<2>(fInterface) );
/// }
/// \endcode
///
template <viskores::IdComponent ParameterIndex, typename FunctionSignature>
VISKORES_EXEC_CONT auto ParameterGet(const FunctionInterface<FunctionSignature>& fInterface)
  -> decltype(detail::ParameterGet(fInterface.Parameters,
                                   viskores::internal::IndexTag<ParameterIndex>{}))
{
  return detail::ParameterGet(fInterface.Parameters,
                              viskores::internal::IndexTag<ParameterIndex>{});
}


//============================================================================
/// \brief Create a \c FunctionInterface
///
/// \c make_FunctionInterface is a function that takes a variable number of
/// arguments and returns a \c FunctionInterface object containing these
/// objects. Since the return type for the function signature is not specified,
/// you must always specify it as a template parameter
///
/// \code{.cpp}
/// viskores::internal::FunctionInterface<void(int,double,char)> functionInterface =
///     viskores::internal::make_FunctionInterface<void>(1, 2.5, 'a');
/// \endcode
///

template <typename R, typename... Args>
FunctionInterface<R(Args...)> make_FunctionInterface(const Args&... args)
{
// MSVC will issue deprecation warnings if this templated method is instantiated with
// a deprecated class here even if the method is called from a section of code where
// deprecation warnings are suppressed. This is annoying behavior since this templated
// method has no control over what class it is used from. To get around it, we have to
// suppress all deprecation warnings here.
#ifdef VISKORES_MSVC
  VISKORES_DEPRECATED_SUPPRESS_BEGIN
#endif
  detail::ParameterContainer<R(Args...)> container = { args... };
  return FunctionInterface<R(Args...)>{ container };
#ifdef VISKORES_MSVC
  VISKORES_DEPRECATED_SUPPRESS_END
#endif
}
}
} // namespace viskores::internal

#include <viskores/internal/FunctionInterfaceDetailPost.h>

#endif //viskores_internal_FunctionInterface_h
