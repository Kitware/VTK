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
#ifndef viskores_worklet_Dispatcher_MapTopology_h
#define viskores_worklet_Dispatcher_MapTopology_h

#include <viskores/TopologyElementTag.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/worklet/internal/DispatcherBase.h>

namespace viskores
{
namespace worklet
{
namespace detail
{
struct WorkletMapTopologyBase;
}
class WorkletVisitCellsWithPoints;
class WorkletVisitPointsWithCells;

/// \brief Dispatcher for worklets that inherit from \c WorkletMapTopology.
///
template <typename WorkletType>
class DispatcherMapTopology
  : public viskores::worklet::internal::DispatcherBase<
      DispatcherMapTopology<WorkletType>,
      WorkletType,
      viskores::worklet::detail::WorkletMapTopologyBase>
{
  using Superclass =
    viskores::worklet::internal::DispatcherBase<DispatcherMapTopology<WorkletType>,
                                                WorkletType,
                                                viskores::worklet::detail::WorkletMapTopologyBase>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VISKORES_CONT DispatcherMapTopology(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename Invocation>
  VISKORES_CONT void DoInvoke(Invocation& invocation) const
  {
    using namespace viskores::worklet::internal;

    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;
    using SchedulingRangeType = typename WorkletType::VisitTopologyType;

    // If you get a compile error on this line, then you have tried to use
    // something that is not a viskores::cont::CellSet as the input domain to a
    // topology operation (that operates on a cell set connection domain).
    VISKORES_IS_CELL_SET(InputDomainType);

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const auto& inputDomain = invocation.GetInputDomain();

    // Now that we have the input domain, we can extract the range of the
    // scheduling and call BadicInvoke.
    this->BasicInvoke(invocation, SchedulingRange(inputDomain, SchedulingRangeType{}));
  }
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_Dispatcher_MapTopology_h
