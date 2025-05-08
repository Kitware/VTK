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
#ifndef viskores_exec_arg_ThreadIndicesPointNeighborhood_h
#define viskores_exec_arg_ThreadIndicesPointNeighborhood_h

#include <viskores/exec/arg/ThreadIndicesNeighborhood.h>

namespace viskores
{
namespace exec
{
namespace arg
{
/// \brief Container for thread information in a WorkletPointNeighborhood.
///
///
class ThreadIndicesPointNeighborhood : public viskores::exec::arg::ThreadIndicesNeighborhood
{
  using Superclass = viskores::exec::arg::ThreadIndicesNeighborhood;

public:
  template <viskores::IdComponent Dimension>
  VISKORES_EXEC ThreadIndicesPointNeighborhood(
    const viskores::Id3& threadIndex3D,
    viskores::Id threadIndex1D,
    const viskores::exec::ConnectivityStructured<viskores::TopologyElementTagPoint,
                                                 viskores::TopologyElementTagCell,
                                                 Dimension>& connectivity)
    : Superclass(threadIndex1D,
                 viskores::exec::BoundaryState{ threadIndex3D,
                                                detail::To3D(connectivity.GetPointDimensions()) })
  {
  }

  template <viskores::IdComponent Dimension>
  VISKORES_EXEC ThreadIndicesPointNeighborhood(
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
                                                detail::To3D(connectivity.GetPointDimensions()) })
  {
  }

  template <viskores::IdComponent Dimension>
  VISKORES_EXEC ThreadIndicesPointNeighborhood(
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
                   detail::To3D(connectivity.GetPointDimensions()) })
  {
  }
};
} // arg
} // exec
} // viskores
#endif //viskores_exec_arg_ThreadIndicesPointNeighborhood_h
