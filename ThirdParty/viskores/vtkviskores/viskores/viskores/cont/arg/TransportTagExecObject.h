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
#ifndef viskores_cont_arg_TransportTagExecObject_h
#define viskores_cont_arg_TransportTagExecObject_h

#include <viskores/Types.h>

#include <viskores/cont/arg/Transport.h>

#include <viskores/cont/ExecutionObjectBase.h>


namespace viskores
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for execution objects.
///
/// \c TransportTagExecObject is a tag used with the \c Transport class to
/// transport objects that work directly in the execution environment.
///
struct TransportTagExecObject
{
};

template <typename ContObjectType, typename Device>
struct Transport<viskores::cont::arg::TransportTagExecObject, ContObjectType, Device>
{
  // If you get a compile error here, it means you tried to use an object that is not an execution
  // object as an argument that is expected to be one. All execution objects are expected to
  // inherit from viskores::cont::ExecutionObjectBase and have a PrepareForExecution method.
  VISKORES_IS_EXECUTION_OBJECT(ContObjectType);

  using ExecObjectType = viskores::cont::internal::ExecutionObjectType<ContObjectType, Device>;
  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(ContObjectType& object,
                                          const InputDomainType&,
                                          viskores::Id,
                                          viskores::Id,
                                          viskores::cont::Token& token) const
  {
    return viskores::cont::internal::CallPrepareForExecution(object, Device{}, token);
  }
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TransportTagExecObject_h
