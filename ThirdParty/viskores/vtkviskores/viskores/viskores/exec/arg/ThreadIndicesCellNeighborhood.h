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
#ifndef viskores_exec_arg_ThreadIndicesCellNeighborhood_h
#define viskores_exec_arg_ThreadIndicesCellNeighborhood_h

#include <viskores/exec/BoundaryState.h>
#include <viskores/exec/ConnectivityStructured.h>
#include <viskores/exec/arg/ThreadIndicesBasic.h>
#include <viskores/exec/arg/ThreadIndicesNeighborhood.h>
#include <viskores/exec/arg/ThreadIndicesTopologyMap.h> //for Deflate and Inflate

#include <viskores/Math.h>

namespace viskores
{
namespace exec
{
namespace arg
{
/// \brief Container for thread information in a WorkletCellNeighborhood.
///
///
class ThreadIndicesCellNeighborhood : public viskores::exec::arg::ThreadIndicesNeighborhood
{
  using Superclass = viskores::exec::arg::ThreadIndicesNeighborhood;

public:
  template <viskores::IdComponent Dimension>
  VISKORES_EXEC ThreadIndicesCellNeighborhood(
    const viskores::Id3& threadIndex3D,
    viskores::Id threadIndex1D,
    const viskores::exec::ConnectivityStructured<viskores::TopologyElementTagPoint,
                                                 viskores::TopologyElementTagCell,
                                                 Dimension>& connectivity)
    : Superclass(threadIndex1D,
                 viskores::exec::BoundaryState{ threadIndex3D,
                                                detail::To3D(connectivity.GetCellDimensions()) })
  {
  }

  template <viskores::IdComponent Dimension>
  VISKORES_EXEC ThreadIndicesCellNeighborhood(
    const viskores::Id3& threadIndex3D,
    viskores::Id threadIndex1D,
    viskores::Id inputIndex,
    viskores::IdComponent visitIndex,
    viskores::Id outputIndex,
    const viskores::exec::ConnectivityStructured<viskores::TopologyElementTagPoint,
                                                 viskores::TopologyElementTagCell,
                                                 Dimension>& connectivity)
    : Superclass(threadIndex1D,
                 inputIndex,
                 visitIndex,
                 outputIndex,
                 viskores::exec::BoundaryState{ threadIndex3D,
                                                detail::To3D(connectivity.GetCellDimensions()) })
  {
  }

  template <viskores::IdComponent Dimension>
  VISKORES_EXEC ThreadIndicesCellNeighborhood(
    viskores::Id threadIndex,
    viskores::Id inputIndex,
    viskores::IdComponent visitIndex,
    viskores::Id outputIndex,
    const viskores::exec::ConnectivityStructured<viskores::TopologyElementTagPoint,
                                                 viskores::TopologyElementTagCell,
                                                 Dimension>& connectivity)
    : Superclass(threadIndex,
                 inputIndex,
                 visitIndex,
                 outputIndex,
                 viskores::exec::BoundaryState{
                   detail::To3D(connectivity.FlatToLogicalVisitIndex(inputIndex)),
                   detail::To3D(connectivity.GetCellDimensions()) })
  {
  }
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_ThreadIndicesCellNeighborhood_h
