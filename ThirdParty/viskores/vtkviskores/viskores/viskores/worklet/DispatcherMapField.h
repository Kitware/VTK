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
#ifndef viskores_worklet_Dispatcher_MapField_h
#define viskores_worklet_Dispatcher_MapField_h

#include <viskores/worklet/internal/DispatcherBase.h>

namespace viskores
{
namespace worklet
{

class WorkletMapField;

/// \brief Dispatcher for worklets that inherit from \c WorkletMapField.
///
template <typename WorkletType>
class DispatcherMapField
  : public viskores::worklet::internal::DispatcherBase<DispatcherMapField<WorkletType>,
                                                       WorkletType,
                                                       viskores::worklet::WorkletMapField>
{
  using Superclass =
    viskores::worklet::internal::DispatcherBase<DispatcherMapField<WorkletType>,
                                                WorkletType,
                                                viskores::worklet::WorkletMapField>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VISKORES_CONT DispatcherMapField(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename Invocation>
  VISKORES_CONT void DoInvoke(Invocation& invocation) const
  {
    using namespace viskores::worklet::internal;

    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // For a DispatcherMapField, the inputDomain must be an ArrayHandle (or
    // an UnknownArrayHandle that gets cast to one). The size of the domain
    // (number of threads/worklet instances) is equal to the size of the
    // array.
    auto numInstances = SchedulingRange(inputDomain);

    // A MapField is a pretty straightforward dispatch. Once we know the number
    // of invocations, the superclass can take care of the rest.
    this->BasicInvoke(invocation, numInstances);
  }
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_Dispatcher_MapField_h
