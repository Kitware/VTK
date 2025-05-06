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
#ifndef viskores_worklet_internal_DispatcherBase_h
#define viskores_worklet_internal_DispatcherBase_h

#include <viskores/List.h>
#include <viskores/StaticAssert.h>

#include <viskores/internal/FunctionInterface.h>
#include <viskores/internal/Invocation.h>

#include <viskores/cont/CastAndCall.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/cont/arg/ControlSignatureTagBase.h>
#include <viskores/cont/arg/Transport.h>
#include <viskores/cont/arg/TypeCheck.h>

#include <viskores/exec/arg/ExecutionSignatureTagBase.h>

#include <viskores/internal/DecayHelpers.h>

#include <viskores/worklet/internal/WorkletBase.h>

#include <viskoresstd/integer_sequence.h>
#include <viskoresstd/is_trivial.h>

#include <sstream>

namespace viskores
{
namespace worklet
{
namespace internal
{

template <typename Domain>
inline auto SchedulingRange(const Domain& inputDomain) -> decltype(inputDomain.GetNumberOfValues())
{
  return inputDomain.GetNumberOfValues();
}

template <typename Domain>
inline auto SchedulingRange(const Domain* const inputDomain)
  -> decltype(inputDomain->GetNumberOfValues())
{
  return inputDomain->GetNumberOfValues();
}

template <typename Domain, typename SchedulingRangeType>
inline auto SchedulingRange(const Domain& inputDomain, SchedulingRangeType type)
  -> decltype(inputDomain.GetSchedulingRange(type))
{
  return inputDomain.GetSchedulingRange(type);
}

template <typename Domain, typename SchedulingRangeType>
inline auto SchedulingRange(const Domain* const inputDomain, SchedulingRangeType type)
  -> decltype(inputDomain->GetSchedulingRange(type))
{
  return inputDomain->GetSchedulingRange(type);
}

namespace detail
{

// This code is actually taking an error found at compile-time and not
// reporting it until run-time. This seems strange at first, but this
// behavior is actually important. With dynamic arrays and similar dynamic
// classes, there may be types that are technically possible (such as using a
// vector where a scalar is expected) but in reality never happen. Thus, for
// these unsupported combinations we just silently halt the compiler from
// attempting to create code for these errant conditions and throw a run-time
// error if one every tries to create one.
inline void PrintFailureMessage(int index)
{
  std::stringstream message;
  message << "Encountered bad type for parameter " << index
          << " when calling Invoke on a dispatcher.";
  throw viskores::cont::ErrorBadType(message.str());
}

inline void PrintNullPtrMessage(int index, int mode)
{
  std::stringstream message;
  if (mode == 0)
  {
    message << "Encountered nullptr for parameter " << index;
  }
  else
  {
    message << "Encountered nullptr for " << index << " from last parameter ";
  }
  message << " when calling Invoke on a dispatcher.";
  throw viskores::cont::ErrorBadValue(message.str());
}

template <typename T>
inline void not_nullptr(T* ptr, int index, int mode = 0)
{
  if (!ptr)
  {
    PrintNullPtrMessage(index, mode);
  }
}
template <typename T>
inline void not_nullptr(T&&, int, int mode = 0)
{
  (void)mode;
}

template <typename T>
inline T& as_ref(T* ptr)
{
  return *ptr;
}
template <typename T>
inline T&& as_ref(T&& t)
{
  return std::forward<T>(t);
}


template <typename T, bool noError>
struct ReportTypeOnError : std::integral_constant<bool, noError>
{
};

template <int Value, bool noError>
struct ReportValueOnError : std::integral_constant<bool, noError>
{
};

template <typename Type>
struct IsDynamicTypeImpl
{
  using T = viskores::internal::remove_pointer_and_decay<Type>;
  using DynamicTag = typename viskores::cont::internal::DynamicTransformTraits<T>::DynamicTag;
  using type =
    typename std::is_same<DynamicTag,
                          viskores::cont::internal::DynamicTransformTagCastAndCall>::type;
};
template <typename T>
using IsDynamicType = typename IsDynamicTypeImpl<T>::type;

template <typename SigTagList, typename ParamList, typename IndexSequence>
struct ZipControlParamImpl;
template <typename... SigTags, typename... Params, viskores::IdComponent... Indices>
struct ZipControlParamImpl<viskores::List<SigTags...>,
                           viskores::List<Params...>,
                           viskoresstd::integer_sequence<viskores::IdComponent, Indices...>>
{
  VISKORES_STATIC_ASSERT(sizeof...(SigTags) == sizeof...(Params));
  VISKORES_STATIC_ASSERT(sizeof...(SigTags) == sizeof...(Indices));

  using type =
    viskores::List<viskores::List<SigTags,
                                  Params,
                                  std::integral_constant<viskores::IdComponent, (Indices + 1)>>...>;
};
template <typename SigTagList, typename ParamList, typename IndexSequence>
using ZipControlParam = typename ZipControlParamImpl<SigTagList, ParamList, IndexSequence>::type;

template <typename WorkletType>
struct ControlArgumentValidator
{
  template <typename SigTag, typename Param, viskores::IdComponent Index>
  void operator()(
    viskores::List<SigTag, Param, std::integral_constant<viskores::IdComponent, Index>>) const
  {
    using T = std::remove_pointer_t<Param>;
    using TypeCheckTag = typename SigTag::TypeCheckTag;
    static constexpr bool isCorrect = viskores::cont::arg::TypeCheck<TypeCheckTag, T>::value;

    static_assert(
      isCorrect,
      "If you get an error here, that means that your code has invoked a worklet,"
      " and one of the arguments of the Invoke is the wrong type. Each argument of the invoke"
      " corresponds to a tag in the arguments of the ControlSignature of the worklet. If there"
      " is a mismatch, then you get an error here (instead of where you called invoke). For"
      " example, if the worklet has a control signature as ControlSignature(CellSetIn, ...) and"
      " the first argument passed to the invoke is an ArrayHandle, you will get an error here"
      " because you cannot use an ArrayHandle in place of a CellSetIn argument. (You need to use"
      " a CellSet.) If the compiler supports it, the next few errors on the following lines of"
      " code will give information about where the error actually occurred.");

    // If you are getting the error described above, the following lines will give you some
    // diagnostics (in the form of compile errors). Each one will result in a compile error
    // reporting an undefined type for ReportTypeOnError (or ReportValueOnError). What we are
    // really reporting is the first template argument, which is one of the types or values that
    // should help pinpoint where the error is. The comment for static_assert provides the
    // type/value being reported. (Note that some compilers report better types than others. If
    // your compiler is giving unhelpful types like "T" or "WorkletType", you may need to try a
    // different compiler.)
    static_assert(ReportTypeOnError<WorkletType, isCorrect>::value,
                  "The first template argument to ReportTypeOnError is the worklet being invoked");
    static_assert(ReportValueOnError<Index, isCorrect>::value,
                  "The first template argument to ReportTypeOnError is the index of Invoke "
                  "parameter (starting at index 1)");
    static_assert(ReportTypeOnError<T, isCorrect>::value,
                  "The first template argument to ReportTypeOnError is the type passed to Invoke");
    static_assert(ReportTypeOnError<TypeCheckTag, isCorrect>::value,
                  "The first template argument to ReportTypeOnError is the type check tag used");
  }
};

// Checks that an argument in a ControlSignature is a valid control signature
// tag. Causes a compile error otherwise.
struct DispatcherBaseControlSignatureTagCheck
{
  template <typename ControlSignatureTag, viskores::IdComponent Index>
  struct ReturnType
  {
    // If you get a compile error here, it means there is something that is
    // not a valid control signature tag in a worklet's ControlSignature.
    VISKORES_IS_CONTROL_SIGNATURE_TAG(ControlSignatureTag);
    using type = ControlSignatureTag;
  };
};

// Checks that an argument in a ExecutionSignature is a valid execution
// signature tag. Causes a compile error otherwise.
struct DispatcherBaseExecutionSignatureTagCheck
{
  template <typename ExecutionSignatureTag, viskores::IdComponent Index>
  struct ReturnType
  {
    // If you get a compile error here, it means there is something that is not
    // a valid execution signature tag in a worklet's ExecutionSignature.
    VISKORES_IS_EXECUTION_SIGNATURE_TAG(ExecutionSignatureTag);
    using type = ExecutionSignatureTag;
  };
};

struct DispatcherBaseTryExecuteFunctor
{
  template <typename Device, typename DispatcherBaseType, typename Invocation, typename RangeType>
  VISKORES_CONT bool operator()(Device device,
                                const DispatcherBaseType* self,
                                Invocation& invocation,
                                const RangeType& dimensions)
  {
    auto outputRange = self->Scatter.GetOutputRange(dimensions);
    self->InvokeTransportParameters(
      invocation, dimensions, outputRange, self->Mask.GetThreadRange(outputRange), device);
    return true;
  }
};

// A look up helper used by DispatcherBaseTransportFunctor to determine
//the types independent of the device we are templated on.
template <typename ControlInterface, viskores::IdComponent Index>
struct DispatcherBaseTransportInvokeTypes
{
  //Moved out of DispatcherBaseTransportFunctor to reduce code generation
  using ControlSignatureTag = typename ControlInterface::template ParameterType<Index>::type;
  using TransportTag = typename ControlSignatureTag::TransportTag;
};

VISKORES_CONT
inline viskores::Id FlatRange(viskores::Id range)
{
  return range;
}

VISKORES_CONT
inline viskores::Id FlatRange(const viskores::Id3& range)
{
  return range[0] * range[1] * range[2];
}

// A functor used in a StaticCast of a FunctionInterface to transport arguments
// from the control environment to the execution environment.
template <typename ControlInterface, typename InputDomainType, typename Device>
struct DispatcherBaseTransportFunctor
{
  const InputDomainType& InputDomain; // Warning: this is a reference
  viskores::Id InputRange;
  viskores::Id OutputRange;
  viskores::cont::Token& Token; // Warning: this is a reference

  // TODO: We need to think harder about how scheduling on 3D arrays works.
  // Chances are we need to allow the transport for each argument to manage
  // 3D indices (for example, allocate a 3D array instead of a 1D array).
  // But for now, just treat all transports as 1D arrays.
  template <typename InputRangeType, typename OutputRangeType>
  VISKORES_CONT DispatcherBaseTransportFunctor(const InputDomainType& inputDomain,
                                               const InputRangeType& inputRange,
                                               const OutputRangeType& outputRange,
                                               viskores::cont::Token& token)
    : InputDomain(inputDomain)
    , InputRange(FlatRange(inputRange))
    , OutputRange(FlatRange(outputRange))
    , Token(token)
  {
  }


  template <typename ControlParameter, viskores::IdComponent Index>
  struct ReturnType
  {
    using TransportTag =
      typename DispatcherBaseTransportInvokeTypes<ControlInterface, Index>::TransportTag;
    using T = viskores::internal::remove_pointer_and_decay<ControlParameter>;
    using TransportType = typename viskores::cont::arg::Transport<TransportTag, T, Device>;
    using type = typename std::decay<typename TransportType::ExecObjectType>::type;

    // If you get a compile error here, it means that an execution object type is not
    // trivially copyable. This is strictly disallowed. All execution objects must be
    // trivially copyable so that the can be memcpy-ed between host and devices.
    // Note that it is still legal for execution objects to have pointers or other
    // references to resources on a particular device. It is up to the generating code
    // to ensure that all referenced resources are valid on the target device.
    VISKORES_IS_TRIVIALLY_COPYABLE(type);
  };

  template <typename ControlParameter, viskores::IdComponent Index>
  VISKORES_CONT typename ReturnType<ControlParameter, Index>::type operator()(
    ControlParameter&& invokeData,
    viskores::internal::IndexTag<Index>) const
  {
    using TransportTag =
      typename DispatcherBaseTransportInvokeTypes<ControlInterface, Index>::TransportTag;
    using T = viskores::internal::remove_pointer_and_decay<ControlParameter>;
    viskores::cont::arg::Transport<TransportTag, T, Device> transport;

    not_nullptr(invokeData, Index);
    return transport(as_ref(invokeData),
                     as_ref(this->InputDomain),
                     this->InputRange,
                     this->OutputRange,
                     this->Token);
  }



private:
  void operator=(const DispatcherBaseTransportFunctor&) = delete;
};

// Should this functionality be added to List.h? Should there be the general ability to
// remove some number of items from the beginning or end of a list?
template <typename L>
struct ListRemoveFirstImpl;
template <typename T, typename... Ts>
struct ListRemoveFirstImpl<viskores::List<T, Ts...>>
{
  using type = viskores::List<Ts...>;
};
template <typename L>
using ListRemoveFirst = typename ListRemoveFirstImpl<L>::type;

//forward declares
template <std::size_t LeftToProcess>
struct for_each_dynamic_arg;

template <std::size_t LeftToProcess, typename TypeCheckTag>
struct convert_arg_wrapper
{
  template <typename T, typename... Args>
  void operator()(T&& t, Args&&... args) const
  {
    using Type = typename std::decay<T>::type;
    using valid =
      std::integral_constant<bool, viskores::cont::arg::TypeCheck<TypeCheckTag, Type>::value>;
    this->WillContinue(valid(), std::forward<T>(t), std::forward<Args>(args)...);
  }
  template <typename T, typename... Args>
  void WillContinue(std::true_type, T&& t, Args&&... args) const
  {
    for_each_dynamic_arg<LeftToProcess - 1>()(std::forward<Args>(args)..., std::forward<T>(t));
  }
  template <typename... Args>
  void WillContinue(std::false_type, Args&&...) const
  {
    viskores::worklet::internal::detail::PrintFailureMessage(LeftToProcess);
  }
};

template <std::size_t LeftToProcess,
          typename T,
          typename ContParams,
          typename Trampoline,
          typename... Args>
inline void convert_arg(viskores::cont::internal::DynamicTransformTagStatic,
                        T&& t,
                        const ContParams&,
                        const Trampoline& trampoline,
                        Args&&... args)
{ //This is a static array, so just push it to the back
  using popped_sig = ListRemoveFirst<ContParams>;
  for_each_dynamic_arg<LeftToProcess - 1>()(
    trampoline, popped_sig(), std::forward<Args>(args)..., std::forward<T>(t));
}

template <std::size_t LeftToProcess,
          typename T,
          typename ContParams,
          typename Trampoline,
          typename... Args>
inline void convert_arg(viskores::cont::internal::DynamicTransformTagCastAndCall,
                        T&& t,
                        const ContParams&,
                        const Trampoline& trampoline,
                        Args&&... args)
{ //This is something dynamic so cast and call
  using tag_check = typename viskores::ListAt<ContParams, 0>::TypeCheckTag;
  using popped_sig = ListRemoveFirst<ContParams>;

  not_nullptr(t, LeftToProcess, 1);
  viskores::cont::CastAndCall(as_ref(t),
                              convert_arg_wrapper<LeftToProcess, tag_check>(),
                              trampoline,
                              popped_sig(),
                              std::forward<Args>(args)...);
}

template <std::size_t LeftToProcess>
struct for_each_dynamic_arg
{
  template <typename Trampoline, typename ContParams, typename T, typename... Args>
  void operator()(const Trampoline& trampoline, ContParams&& sig, T&& t, Args&&... args) const
  {
    //Determine that state of T when it is either a `cons&` or a `* const&`
    using Type = viskores::internal::remove_pointer_and_decay<T>;
    using tag = typename viskores::cont::internal::DynamicTransformTraits<Type>::DynamicTag;
    //convert the first item to a known type
    convert_arg<LeftToProcess>(
      tag(), std::forward<T>(t), sig, trampoline, std::forward<Args>(args)...);
  }
};

template <>
struct for_each_dynamic_arg<0>
{
  template <typename Trampoline, typename ContParams, typename... Args>
  void operator()(const Trampoline& trampoline, ContParams&&, Args&&... args) const
  {
    trampoline.StartInvokeDynamic(std::false_type(), std::forward<Args>(args)...);
  }
};

template <typename Trampoline, typename ContParams, typename... Args>
inline void deduce(Trampoline&& trampoline, ContParams&& sig, Args&&... args)
{
  for_each_dynamic_arg<sizeof...(Args)>()(std::forward<Trampoline>(trampoline), sig, args...);
}

} // namespace detail

/// This is a help struct to detect out of bound placeholders defined in the
/// execution signature at compile time
template <viskores::IdComponent MaxIndexAllowed>
struct PlaceholderValidator
{
  PlaceholderValidator() {}

  // An overload operator to detect possible out of bound placeholder
  template <int N>
  void operator()(viskores::internal::meta::Type<viskores::placeholders::Arg<N>>) const
  {
    static_assert(N <= MaxIndexAllowed,
                  "An argument in the execution signature"
                  " (usually _2, _3, _4, etc.) refers to a control signature argument that"
                  " does not exist. For example, you will get this error if you have _3 (or"
                  " _4 or _5 or so on) as one of the execution signature arguments, but you"
                  " have fewer than 3 (or 4 or 5 or so on) arguments in the control signature.");
  }

  template <typename DerivedType>
  void operator()(viskores::internal::meta::Type<DerivedType>) const
  {
  }
};

/// Base class for all dispatcher classes. Every worklet type should have its
/// own dispatcher.
///
template <typename DerivedClass, typename WorkletType, typename BaseWorkletType>
class DispatcherBase
{
private:
  using MyType = DispatcherBase<DerivedClass, WorkletType, BaseWorkletType>;

  friend struct detail::for_each_dynamic_arg<0>;

protected:
  using ControlInterface =
    viskores::internal::FunctionInterface<typename WorkletType::ControlSignature>;

  // We go through the GetExecSig as that generates a default ExecutionSignature
  // if one doesn't exist on the worklet
  using ExecutionSignature =
    typename viskores::placeholders::GetExecSig<WorkletType>::ExecutionSignature;
  using ExecutionInterface = viskores::internal::FunctionInterface<ExecutionSignature>;

  static constexpr viskores::IdComponent NUM_INVOKE_PARAMS = ControlInterface::ARITY;

private:
  // We don't really need these types, but declaring them checks the arguments
  // of the control and execution signatures.
  using ControlSignatureCheck = typename ControlInterface::template StaticTransformType<
    detail::DispatcherBaseControlSignatureTagCheck>::type;
  using ExecutionSignatureCheck = typename ExecutionInterface::template StaticTransformType<
    detail::DispatcherBaseExecutionSignatureTagCheck>::type;

  template <typename... Args>
  VISKORES_CONT void StartInvoke(Args&&... args) const
  {
    using ParameterInterface =
      viskores::internal::FunctionInterface<void(viskores::internal::remove_cvref<Args>...)>;

    VISKORES_STATIC_ASSERT_MSG(ParameterInterface::ARITY == NUM_INVOKE_PARAMS,
                               "Dispatcher Invoke called with wrong number of arguments.");

    static_assert(
      std::is_base_of<BaseWorkletType, WorkletType>::value,
      "The worklet being scheduled by this dispatcher doesn't match the type of the dispatcher");

    // Check if the placeholders defined in the execution environment exceed the max bound
    // defined in the control environment by throwing a nice compile error.
    using ComponentSig = typename ExecutionInterface::ComponentSig;
    viskores::ListForEach(PlaceholderValidator<NUM_INVOKE_PARAMS>{},
                          viskores::ListTransform<ComponentSig, viskores::internal::meta::Type>{});

    //We need to determine if we have the need to do any dynamic
    //transforms. This is fairly simple of a query. We just need to check
    //everything in the FunctionInterface and see if any of them have the
    //proper dynamic trait. Doing this, allows us to generate zero dynamic
    //check & convert code when we already know all the types. This results
    //in smaller executables and libraries.
    using ParamTypes = typename ParameterInterface::ParameterSig;
    using HasDynamicTypes = viskores::ListAny<ParamTypes, detail::IsDynamicType>;

    this->StartInvokeDynamic(HasDynamicTypes(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  VISKORES_CONT void StartInvokeDynamic(std::true_type, Args&&... args) const
  {
    // As we do the dynamic transform, we are also going to check the static
    // type against the TypeCheckTag in the ControlSignature tags. To do this,
    // the check needs access to both the parameter (in the parameters
    // argument) and the ControlSignature tags (in the ControlInterface type).
    using ContParamsInfo =
      viskores::internal::detail::FunctionSigInfo<typename WorkletType::ControlSignature>;
    typename ContParamsInfo::Parameters parameters;
    detail::deduce(*this, parameters, std::forward<Args>(args)...);
  }

  template <typename... Args>
  VISKORES_CONT void StartInvokeDynamic(std::false_type, Args&&... args) const
  {
    using ParameterInterface =
      viskores::internal::FunctionInterface<void(viskores::internal::remove_cvref<Args>...)>;

    //Nothing requires a conversion from dynamic to static types, so
    //next we need to verify that each argument's type is correct. If not
    //we need to throw a nice compile time error
    using ParamTypes = typename ParameterInterface::ParameterSig;
    using ContSigTypes = typename viskores::internal::detail::FunctionSigInfo<
      typename WorkletType::ControlSignature>::Parameters;
    using ParamZip = detail::ZipControlParam<
      ContSigTypes,
      ParamTypes,
      viskoresstd::make_integer_sequence<viskores::IdComponent,
                                         viskores::ListSize<ParamTypes>::value>>;

    // This will cause compile errors if there is an argument mismatch.
    viskores::ListForEach(detail::ControlArgumentValidator<WorkletType>{}, ParamZip{});

    auto fi =
      viskores::internal::make_FunctionInterface<void, viskores::internal::remove_cvref<Args>...>(
        args...);
    auto ivc = viskores::internal::Invocation<ParameterInterface,
                                              ControlInterface,
                                              ExecutionInterface,
                                              WorkletType::InputDomain::INDEX,
                                              viskores::internal::NullType,
                                              viskores::internal::NullType>(
      fi, viskores::internal::NullType{}, viskores::internal::NullType{});
    static_cast<const DerivedClass*>(this)->DoInvoke(ivc);
  }

public:
  ///@{
  /// Setting the device ID will force the execute to happen on a particular device. If no device
  /// is specified (or the device ID is set to any), then a device will automatically be chosen
  /// based on the runtime device tracker.
  ///
  VISKORES_CONT
  void SetDevice(viskores::cont::DeviceAdapterId device) { this->Device = device; }

  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const { return this->Device; }
  ///@}

  using ScatterType = typename WorkletType::ScatterType;
  using MaskType = typename WorkletType::MaskType;

  template <typename... Args>
  VISKORES_CONT void Invoke(Args&&... args) const
  {
    VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf,
                       "Invoking Worklet: '%s'",
                       viskores::cont::TypeToString<DerivedClass>().c_str());
    this->StartInvoke(std::forward<Args>(args)...);
  }

protected:
  // If you get a compile error here about there being no appropriate constructor for ScatterType
  // or MapType, then that probably means that the worklet you are trying to execute has defined a
  // custom ScatterType or MaskType and that you need to create one (because there is no default
  // way to construct the scatter or mask).
  VISKORES_CONT
  DispatcherBase(const WorkletType& worklet = WorkletType(),
                 const ScatterType& scatter = ScatterType(),
                 const MaskType& mask = MaskType())
    : Worklet(worklet)
    , Scatter(scatter)
    , Mask(mask)
    , Device(viskores::cont::DeviceAdapterTagAny())
  {
  }

  // If you get a compile error here about there being no appropriate constructor for MaskType,
  // then that probably means that the worklet you are trying to execute has defined a custom
  // MaskType and that you need to create one (because there is no default way to construct the
  // mask).
  VISKORES_CONT
  DispatcherBase(const ScatterType& scatter, const MaskType& mask = MaskType())
    : Worklet(WorkletType())
    , Scatter(scatter)
    , Mask(mask)
    , Device(viskores::cont::DeviceAdapterTagAny())
  {
  }

  // If you get a compile error here about there being no appropriate constructor for ScatterType,
  // then that probably means that the worklet you are trying to execute has defined a custom
  // ScatterType and that you need to create one (because there is no default way to construct the
  // scatter).
  VISKORES_CONT
  DispatcherBase(const WorkletType& worklet,
                 const MaskType& mask,
                 const ScatterType& scatter = ScatterType())
    : Worklet(worklet)
    , Scatter(scatter)
    , Mask(mask)
    , Device(viskores::cont::DeviceAdapterTagAny())
  {
  }

  // If you get a compile error here about there being no appropriate constructor for ScatterType,
  // then that probably means that the worklet you are trying to execute has defined a custom
  // ScatterType and that you need to create one (because there is no default way to construct the
  // scatter).
  VISKORES_CONT
  DispatcherBase(const MaskType& mask, const ScatterType& scatter = ScatterType())
    : Worklet(WorkletType())
    , Scatter(scatter)
    , Mask(mask)
    , Device(viskores::cont::DeviceAdapterTagAny())
  {
  }

  friend struct internal::detail::DispatcherBaseTryExecuteFunctor;

  template <typename Invocation>
  VISKORES_CONT void BasicInvoke(Invocation& invocation, viskores::Id numInstances) const
  {
    bool success =
      viskores::cont::TryExecuteOnDevice(this->Device,
                                         internal::detail::DispatcherBaseTryExecuteFunctor(),
                                         this,
                                         invocation,
                                         numInstances);
    if (!success)
    {
      throw viskores::cont::ErrorExecution("Failed to execute worklet on any device.");
    }
  }

  template <typename Invocation>
  VISKORES_CONT void BasicInvoke(Invocation& invocation, viskores::Id2 dimensions) const
  {
    this->BasicInvoke(invocation, viskores::Id3(dimensions[0], dimensions[1], 1));
  }

  template <typename Invocation>
  VISKORES_CONT void BasicInvoke(Invocation& invocation, viskores::Id3 dimensions) const
  {
    bool success =
      viskores::cont::TryExecuteOnDevice(this->Device,
                                         internal::detail::DispatcherBaseTryExecuteFunctor(),
                                         this,
                                         invocation,
                                         dimensions);
    if (!success)
    {
      throw viskores::cont::ErrorExecution("Failed to execute worklet on any device.");
    }
  }

  WorkletType Worklet;
  ScatterType Scatter;
  MaskType Mask;

private:
  // Dispatchers cannot be copied
  DispatcherBase(const MyType&) = delete;
  void operator=(const MyType&) = delete;

  viskores::cont::DeviceAdapterId Device;

  template <typename Invocation,
            typename InputRangeType,
            typename OutputRangeType,
            typename ThreadRangeType,
            typename DeviceAdapter>
  VISKORES_CONT void InvokeTransportParameters(Invocation& invocation,
                                               const InputRangeType& inputRange,
                                               OutputRangeType&& outputRange,
                                               ThreadRangeType&& threadRange,
                                               DeviceAdapter device) const
  {
    // This token represents the scope of the execution objects. It should
    // exist as long as things run on the device.
    viskores::cont::Token token;

    // The first step in invoking a worklet is to transport the arguments to
    // the execution environment. The invocation object passed to this function
    // contains the parameters passed to Invoke in the control environment. We
    // will use the template magic in the FunctionInterface class to invoke the
    // appropriate Transport class on each parameter and get a list of
    // execution objects (corresponding to the arguments of the Invoke in the
    // control environment) in a FunctionInterface. Specifically, we use a
    // static transform of the FunctionInterface to call the transport on each
    // argument and return the corresponding execution environment object.
    using ParameterInterfaceType = typename Invocation::ParameterInterface;
    ParameterInterfaceType& parameters = invocation.Parameters;

    using TransportFunctorType =
      detail::DispatcherBaseTransportFunctor<typename Invocation::ControlInterface,
                                             typename Invocation::InputDomainType,
                                             DeviceAdapter>;
    using ExecObjectParameters =
      typename ParameterInterfaceType::template StaticTransformType<TransportFunctorType>::type;

    ExecObjectParameters execObjectParameters = parameters.StaticTransformCont(
      TransportFunctorType(invocation.GetInputDomain(), inputRange, outputRange, token));

    // Get the arrays used for scattering input to output.
    typename ScatterType::OutputToInputMapType outputToInputMap =
      this->Scatter.GetOutputToInputMap(inputRange);
    typename ScatterType::VisitArrayType visitArray = this->Scatter.GetVisitArray(inputRange);

    // Get the arrays used for masking output elements.
    typename MaskType::ThreadToOutputMapType threadToOutputMap =
      this->Mask.GetThreadToOutputMap(outputRange);

    // Replace the parameters in the invocation with the execution object and
    // pass to next step of Invoke. Also add the scatter information.
    viskores::internal::Invocation<ExecObjectParameters,
                                   typename Invocation::ControlInterface,
                                   typename Invocation::ExecutionInterface,
                                   Invocation::InputDomainIndex,
                                   decltype(outputToInputMap.PrepareForInput(device, token)),
                                   decltype(visitArray.PrepareForInput(device, token)),
                                   decltype(threadToOutputMap.PrepareForInput(device, token)),
                                   DeviceAdapter>
      changedInvocation(execObjectParameters,
                        outputToInputMap.PrepareForInput(device, token),
                        visitArray.PrepareForInput(device, token),
                        threadToOutputMap.PrepareForInput(device, token));

    this->InvokeSchedule(changedInvocation, threadRange, device);
  }

  template <typename Invocation, typename RangeType, typename DeviceAdapter>
  VISKORES_CONT void InvokeSchedule(const Invocation& invocation,
                                    RangeType range,
                                    DeviceAdapter) const
  {
    using Algorithm = viskores::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    using TaskTypes = typename viskores::cont::DeviceTaskTypes<DeviceAdapter>;

    // The TaskType class handles the magic of fetching values
    // for each instance and calling the worklet's function.
    // The TaskType will evaluate to one of the following classes:
    //
    // viskores::exec::internal::TaskSingular
    // viskores::exec::internal::TaskTiling1D
    // viskores::exec::internal::TaskTiling3D
    auto task =
      TaskTypes::MakeTask(this->Worklet, invocation, range, typename WorkletType::Hints{});
    Algorithm::ScheduleTask(task, range);
  }
};
}
}
} // namespace viskores::worklet::internal

#endif //viskores_worklet_internal_DispatcherBase_h
