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
#ifndef viskores_cont_ExecutionObjectBase_h
#define viskores_cont_ExecutionObjectBase_h

#include <viskores/Types.h>

#include <viskores/cont/Token.h>

#include <viskores/cont/serial/internal/DeviceAdapterTagSerial.h>

namespace viskores
{
namespace cont
{

/// Base `ExecutionObjectBase` for execution objects to inherit from so that
/// you can use an arbitrary object as a parameter in an execution environment
/// function. Any subclass of `ExecutionObjectBase` must implement a
/// `PrepareForExecution` method that takes a device adapter tag and a
/// `viskores::cont::Token` and then returns an object for that device. The object
/// must be valid as long as the `Token` is in scope.
///
struct ExecutionObjectBase
{
};

namespace internal
{

namespace detail
{

struct CheckPrepareForExecution
{
  template <typename T>
  static auto check(T* p)
    -> decltype(p->PrepareForExecution(viskores::cont::DeviceAdapterTagSerial{},
                                       std::declval<viskores::cont::Token&>()),
                std::true_type());

  template <typename T>
  static auto check(...) -> std::false_type;
};

} // namespace detail

template <typename T>
using IsExecutionObjectBase =
  typename std::is_base_of<viskores::cont::ExecutionObjectBase, typename std::decay<T>::type>::type;

template <typename T>
struct HasPrepareForExecution
  : decltype(detail::CheckPrepareForExecution::check<typename std::decay<T>::type>(nullptr))
{
};

/// Checks that the argument is a proper execution object.
///
#define VISKORES_IS_EXECUTION_OBJECT(execObject)                                            \
  static_assert(::viskores::cont::internal::IsExecutionObjectBase<execObject>::value,       \
                "Provided type is not a subclass of viskores::cont::ExecutionObjectBase."); \
  static_assert(::viskores::cont::internal::HasPrepareForExecution<execObject>::value,      \
                "Provided type does not have requisite PrepareForExecution method.")

///@{
/// \brief Gets the object to use in the execution environment from an ExecutionObject.
///
/// An execution object (that is, an object inheriting from `viskores::cont::ExecutionObjectBase`) is
/// really a control object factory that generates an object to be used in the execution
/// environment for a particular device. This function takes a subclass of `ExecutionObjectBase`
/// and returns the execution object for a given device.
///
template <typename T, typename Device>
VISKORES_CONT auto CallPrepareForExecution(T&& execObject,
                                           Device device,
                                           viskores::cont::Token& token)
  -> decltype(execObject.PrepareForExecution(device, token))
{
  VISKORES_IS_EXECUTION_OBJECT(T);
  VISKORES_IS_DEVICE_ADAPTER_TAG(Device);

  return execObject.PrepareForExecution(device, token);
}

template <typename T>
VISKORES_CONT auto CallPrepareForExecution(T&& execObject,
                                           viskores::cont::DeviceAdapterId device,
                                           viskores::cont::Token& token)
  -> decltype(execObject.PrepareForExecution(device, token))
{
  VISKORES_IS_EXECUTION_OBJECT(T);

  return execObject.PrepareForExecution(device, token);
}
///@}

/// \brief Gets the type of the execution-side object for an ExecutionObject.
///
/// An execution object (that is, an object inheriting from `viskores::cont::ExecutionObjectBase`) is
/// really a control object factory that generates an object to be used in the execution
/// environment for a particular device. This templated type gives the type for the class used
/// in the execution environment for a given ExecutionObject and device.
///
template <typename ExecutionObject, typename Device = viskores::cont::DeviceAdapterId>
using ExecutionObjectType =
  decltype(CallPrepareForExecution(std::declval<ExecutionObject>(),
                                   std::declval<Device>(),
                                   std::declval<viskores::cont::Token&>()));

} // namespace internal
}
} // namespace viskores::cont

#endif //viskores_cont_ExecutionObjectBase_h
