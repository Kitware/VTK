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
#ifndef viskores_exec_arg_FetchExtrude_h
#define viskores_exec_arg_FetchExtrude_h

#include <viskores/exec/ConnectivityExtrude.h>
#include <viskores/exec/arg/FetchTagArrayDirectIn.h>
#include <viskores/exec/arg/FetchTagArrayTopologyMapIn.h>

#include <viskores/cont/ArrayHandleXGCCoordinates.h>

//optimized fetches for ArrayPortalXGCCoordinates for
// - 3D Scheduling
// - WorkletNeighboorhood
namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the visited indices.
///
/// The \c AspectTagIncidentElementIndices aspect tag causes the \c Fetch class
/// to obtain the indices that map to the current topology element.
///
struct AspectTagIncidentElementIndices
{
};

//Optimized fetch for point ids when iterating the cells ConnectivityExtrude
template <typename FetchType, typename ExecObjectType>
struct Fetch<FetchType, viskores::exec::arg::AspectTagIncidentElementIndices, ExecObjectType>
{
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ScatterAndMaskMode>
  VISKORES_EXEC auto Load(
    const viskores::exec::arg::ThreadIndicesTopologyMap<viskores::exec::ConnectivityExtrude,
                                                        ScatterAndMaskMode>& indices,
    const ExecObjectType&) const -> viskores::Vec<viskores::Id, 6>
  {
    // std::cout << "opimized fetch for point ids" << std::endl;
    const auto& xgcidx = indices.GetIndicesIncident();
    const viskores::Id offset1 = (xgcidx.Planes[0] * xgcidx.NumberOfPointsPerPlane);
    const viskores::Id offset2 = (xgcidx.Planes[1] * xgcidx.NumberOfPointsPerPlane);
    viskores::Vec<viskores::Id, 6> result;
    result[0] = offset1 + xgcidx.PointIds[0][0];
    result[1] = offset1 + xgcidx.PointIds[0][1];
    result[2] = offset1 + xgcidx.PointIds[0][2];
    result[3] = offset2 + xgcidx.PointIds[1][0];
    result[4] = offset2 + xgcidx.PointIds[1][1];
    result[5] = offset2 + xgcidx.PointIds[1][2];
    return result;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ConnectivityType, typename ScatterAndMaskMode>
  VISKORES_EXEC auto Load(
    const viskores::exec::arg::ThreadIndicesTopologyMap<ConnectivityType, ScatterAndMaskMode>&
      indices,
    const ExecObjectType&) const -> decltype(indices.GetIndicesIncident())
  {
    return indices.GetIndicesIncident();
  }

  template <typename ThreadIndicesType, typename ValueType>
  VISKORES_EXEC void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op.
  }
};

//Optimized fetch for point coordinates when iterating the cells of ConnectivityExtrude
template <typename T>
struct Fetch<viskores::exec::arg::FetchTagArrayTopologyMapIn,
             viskores::exec::arg::AspectTagDefault,
             viskores::internal::ArrayPortalXGCCoordinates<T>>

{
  //Optimized fetch for point arrays when iterating the cells ConnectivityExtrude
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ScatterAndMaskMode>
  VISKORES_EXEC auto Load(
    const viskores::exec::arg::ThreadIndicesTopologyMap<viskores::exec::ConnectivityExtrude,
                                                        ScatterAndMaskMode>& indices,
    const viskores::internal::ArrayPortalXGCCoordinates<T>& portal)
    -> decltype(portal.GetWedge(indices.GetIndicesIncident()))
  {
    return portal.GetWedge(indices.GetIndicesIncident());
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC auto Load(const ThreadIndicesType& indices,
                          const viskores::internal::ArrayPortalXGCCoordinates<T>& field) const
    -> decltype(detail::FetchArrayTopologyMapInImplementation<
                typename ThreadIndicesType::Connectivity,
                viskores::internal::ArrayPortalXGCCoordinates<T>,
                ThreadIndicesType>::Load(indices, field))
  {
    using Implementation = detail::FetchArrayTopologyMapInImplementation<
      typename ThreadIndicesType::Connectivity,
      viskores::internal::ArrayPortalXGCCoordinates<T>,
      ThreadIndicesType>;
    return Implementation::Load(indices, field);
  }

  template <typename ThreadIndicesType, typename ValueType>
  VISKORES_EXEC void Store(const ThreadIndicesType&,
                           const viskores::internal::ArrayPortalXGCCoordinates<T>&,
                           const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

//Optimized fetch for point coordinates when iterating the points of ConnectivityExtrude
template <typename T>
struct Fetch<viskores::exec::arg::FetchTagArrayDirectIn,
             viskores::exec::arg::AspectTagDefault,
             viskores::internal::ArrayPortalXGCCoordinates<T>>

{
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC auto Load(const ThreadIndicesType& indices,
                          const viskores::internal::ArrayPortalXGCCoordinates<T>& points)
    -> decltype(points.Get(indices.GetInputIndex()))
  {
    // std::cout << "optimized fetch for point coordinates" << std::endl;
    return points.Get(indices.GetInputIndex());
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ScatterAndMaskMode>
  VISKORES_EXEC auto Load(
    const viskores::exec::arg::ThreadIndicesTopologyMap<viskores::exec::ReverseConnectivityExtrude,
                                                        ScatterAndMaskMode>& indices,
    const viskores::internal::ArrayPortalXGCCoordinates<T>& points)
    -> decltype(points.Get(indices.GetIndexLogical()))
  {
    // std::cout << "optimized fetch for point coordinates" << std::endl;
    return points.Get(indices.GetIndexLogical());
  }

  template <typename ThreadIndicesType, typename ValueType>
  VISKORES_EXEC void Store(const ThreadIndicesType&,
                           const viskores::internal::ArrayPortalXGCCoordinates<T>&,
                           const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
}


#endif
