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
#ifndef viskores_worklet_DispatcherPointNeighborhood_h
#define viskores_worklet_DispatcherPointNeighborhood_h

#include <viskores/cont/DeviceAdapter.h>
#include <viskores/worklet/internal/DispatcherBase.h>

namespace viskores
{
namespace worklet
{
class WorkletNeighborhood;
class WorkletPointNeighborhood;

/// \brief Dispatcher for worklets that inherit from \c WorkletPointNeighborhood.
///
template <typename WorkletType>
class DispatcherPointNeighborhood
  : public viskores::worklet::internal::DispatcherBase<DispatcherPointNeighborhood<WorkletType>,
                                                       WorkletType,
                                                       viskores::worklet::WorkletNeighborhood>
{
  using Superclass =
    viskores::worklet::internal::DispatcherBase<DispatcherPointNeighborhood<WorkletType>,
                                                WorkletType,
                                                viskores::worklet::WorkletNeighborhood>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VISKORES_CONT DispatcherPointNeighborhood(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename Invocation>
  void DoInvoke(Invocation& invocation) const
  {
    using namespace viskores::worklet::internal;

    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // If you get a compile error on this line, then you have tried to use
    // something that is not a viskores::cont::CellSet as the input domain to a
    // topology operation (that operates on a cell set connection domain).
    VISKORES_IS_CELL_SET(InputDomainType);

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();
    auto inputRange = SchedulingRange(inputDomain, viskores::TopologyElementTagPoint{});

    // This is pretty straightforward dispatch. Once we know the number
    // of invocations, the superclass can take care of the rest.
    this->BasicInvoke(invocation, inputRange);
  }
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_DispatcherPointNeighborhood_h
