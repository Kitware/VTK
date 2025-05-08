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
#ifndef viskores_cont_ExecutionAndControlObjectBase_h
#define viskores_cont_ExecutionAndControlObjectBase_h

#include <viskores/cont/ExecutionObjectBase.h>

namespace viskores
{
namespace cont
{

/// Base `ExecutionAndControlObjectBase` class. These are objects that behave
/// as execution objects but can also be use din the control environment.
/// Any subclass of `ExecutionAndControlObjectBase` must implement a
/// `PrepareForExecution` method that takes a device adapter tag and a
/// `viskores::cont::Token` reference and returns
/// an object for that device. It also must implement `PrepareForControl` that simply
/// returns an object that works in the control environment.
///
struct ExecutionAndControlObjectBase : viskores::cont::ExecutionObjectBase
{
};

namespace internal
{

namespace detail
{

struct CheckPrepareForControl
{
  template <typename T>
  static auto check(T* p) -> decltype(p->PrepareForControl(), std::true_type());

  template <typename T>
  static auto check(...) -> std::false_type;
};

} // namespace detail

template <typename T>
using IsExecutionAndControlObjectBase =
  std::is_base_of<viskores::cont::ExecutionAndControlObjectBase, typename std::decay<T>::type>;

template <typename T>
struct HasPrepareForControl
  : decltype(detail::CheckPrepareForControl::check<typename std::decay<T>::type>(nullptr))
{
};

/// Checks that the argument is a proper execution object.
///
#define VISKORES_IS_EXECUTION_AND_CONTROL_OBJECT(execObject)                              \
  static_assert(                                                                          \
    ::viskores::cont::internal::IsExecutionAndControlObjectBase<execObject>::value,       \
    "Provided type is not a subclass of viskores::cont::ExecutionAndControlObjectBase."); \
  static_assert(::viskores::cont::internal::HasPrepareForExecution<execObject>::value,    \
                "Provided type does not have requisite PrepareForExecution method.");     \
  static_assert(::viskores::cont::internal::HasPrepareForControl<execObject>::value,      \
                "Provided type does not have requisite PrepareForControl method.")

/// \brief Gets the object to use in the control environment from an ExecutionAndControlObject.
///
/// An execution and control object (that is, an object inheriting from
/// `viskores::cont::ExecutionAndControlObjectBase`) is really a control object factory that generates
/// a objects to be used in either the execution environment or the control environment. This
/// function takes a subclass of `ExecutionAndControlObjectBase` and returns the control object.
///
template <typename T>
VISKORES_CONT auto CallPrepareForControl(T&& execObject) -> decltype(execObject.PrepareForControl())
{
  VISKORES_IS_EXECUTION_AND_CONTROL_OBJECT(T);

  return execObject.PrepareForControl();
}

/// \brief Gets the object to use in the control environment from an ExecutionAndControlObject.
///
/// An execution and control object (that is, an object inheriting from
/// `viskores::cont::ExecutionAndControlObjectBase`) is really a control object factory that generates
/// a objects to be used in either the execution environment or the control environment. This
/// templated type gives the type for the class used in the control environment for a given
/// ExecutionAndControlObject.
///
template <typename ExecutionAndControlObject>
using ControlObjectType =
  decltype(CallPrepareForControl(std::declval<ExecutionAndControlObject>()));

} // namespace internal
}
} // namespace viskores::cont

#endif //viskores_cont_ExecutionAndControlObjectBase_h
