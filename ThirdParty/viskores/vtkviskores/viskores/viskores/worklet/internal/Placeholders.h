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
#ifndef viskores_worklet_internal_Placeholders_h
#define viskores_worklet_internal_Placeholders_h

#include <viskoresstd/integer_sequence.h>

#include <type_traits>

#include <viskores/exec/arg/BasicArg.h>

namespace viskores
{
namespace placeholders
{

//============================================================================
/// @brief Argument placeholders for an `ExecutionSignature`.
///
/// All worklet superclasses declare numeric tags in the form of
/// `_1`, `_2`, `_3` etc. that are used in the `ExecutionSignature`
/// to refer to the corresponding parameter in the `ControlSignature`.
template <int ControlSignatureIndex>
struct Arg : viskores::exec::arg::BasicArg<ControlSignatureIndex>
{
};

//============================================================================
/**
* Type that computes the number of parameters to the given function signature
*/
template <typename>
struct FunctionSigArity;
template <typename R, typename... ArgTypes>
struct FunctionSigArity<R(ArgTypes...)>
{
  static constexpr std::size_t value = sizeof...(ArgTypes);
};

//============================================================================
template <int... Args>
auto DefaultSigGenerator(viskoresstd::integer_sequence<int, 0, Args...>) -> void (*)(Arg<Args>...);

/**
* Given a desired length will generate the default/assumed ExecutionSignature.
*
* So if you want the ExecutionSignature for a function that has 2 parameters this
* would generate a `type` that is comparable to the user writing:
*
* using ExecutionSignature = void(_1, _2);
*
*/
template <int Length>
struct DefaultExecSig
{
  using seq = viskoresstd::make_integer_sequence<int, Length + 1>;
  using type = typename std::remove_pointer<decltype(DefaultSigGenerator(seq{}))>::type;
};
template <>
struct DefaultExecSig<1>
{
  using type = void(Arg<1>);
};
template <>
struct DefaultExecSig<2>
{
  using type = void(Arg<1>, Arg<2>);
};
template <>
struct DefaultExecSig<3>
{
  using type = void(Arg<1>, Arg<2>, Arg<3>);
};
template <>
struct DefaultExecSig<4>
{
  using type = void(Arg<1>, Arg<2>, Arg<3>, Arg<4>);
};

template <bool HasExecSig_, typename Sig_>
struct ExecSigQuery
{
  static constexpr bool HasExecSig = HasExecSig_;
  using Sig = Sig_;
};

template <typename U, typename S = decltype(std::declval<typename U::ExecutionSignature>())>
static ExecSigQuery<true, typename U::ExecutionSignature> get_exec_sig(int);

template <typename U>
static ExecSigQuery<false, void> get_exec_sig(...);

//============================================================================
/**
* Given a worklet this will produce a typedef `ExecutionSignature` that is
* the ExecutionSignature of the worklet, even if the worklet itself doesn't
* have said typedef.
*
* Logic this class uses:
*
* 1. If the `WorkletType` has a typedef named `ExecutionSignature` use that
* 2. If no typedef exists, generate one!
*   - Presume the Worklet has a `void` return type, and each ControlSignature
*    argument is passed to the worklet in the same listed order.
*   - Generate this assumed `ExecutionSignature` by using  `DefaultExecSig`
*
*/
template <typename WorkletType>
struct GetExecSig
{
  using cont_sig = typename WorkletType::ControlSignature;
  using cont_sig_info = viskores::placeholders::FunctionSigArity<cont_sig>;

  using result = decltype(get_exec_sig<WorkletType>(0));
  static constexpr bool has_explicit_exec_sig = result::HasExecSig;

  using ExecutionSignature = typename std::conditional<
    has_explicit_exec_sig,
    typename result::Sig,
    typename viskores::placeholders::DefaultExecSig<cont_sig_info::value>::type>::type;
};
}
}

#endif
