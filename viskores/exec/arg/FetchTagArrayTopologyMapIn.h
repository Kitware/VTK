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
#ifndef viskores_exec_arg_FetchTagArrayTopologyMapIn_h
#define viskores_exec_arg_FetchTagArrayTopologyMapIn_h

#include <viskores/exec/arg/AspectTagDefault.h>
#include <viskores/exec/arg/Fetch.h>
#include <viskores/exec/arg/ThreadIndicesTopologyMap.h>

#include <viskores/TopologyElementTag.h>

#include <viskores/internal/ArrayPortalUniformPointCoordinates.h>

#include <viskores/VecAxisAlignedPointCoordinates.h>
#include <viskores/exec/ConnectivityExtrude.h>
#include <viskores/exec/ConnectivityStructured.h>

#include <viskores/VecFromPortalPermute.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting array values determined by topology connections.
///
/// \c FetchTagArrayTopologyMapIn is a tag used with the \c Fetch class to
/// retrieve values from an array portal. The fetch uses indexing based on
/// the topology structure used for the input domain.
///
struct FetchTagArrayTopologyMapIn
{
};

/// @cond NONE
namespace detail
{

// This internal class defines how a TopologyMapIn fetch loads from field data
// based on the connectivity class and the object holding the field data. The
// default implementation gets a Vec of indices and an array portal for the
// field and delivers a VecFromPortalPermute. Specializations could have more
// efficient implementations. For example, if the connectivity is structured
// and the field is regular point coordinates, it is much faster to compute the
// field directly.

template <typename ConnectivityType, typename FieldExecObjectType, typename ThreadIndicesType>
struct FetchArrayTopologyMapInImplementation
{
  // stored in a Vec-like object.
  using IndexVecType = typename ThreadIndicesType::IndicesIncidentType;

  // The FieldExecObjectType is expected to behave like an ArrayPortal.
  using PortalType = FieldExecObjectType;

  using ValueType = viskores::VecFromPortalPermute<IndexVecType, PortalType>;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  static ValueType Load(const ThreadIndicesType& indices, const FieldExecObjectType& field)
  {
    // It is important that we give the VecFromPortalPermute (ValueType) a
    // pointer that will stay around during the time the Vec is valid. Thus, we
    // should make sure that indices is a reference that goes up the stack at
    // least as far as the returned VecFromPortalPermute is used.
    return ValueType(indices.GetIndicesIncidentPointer(), field);
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  static ValueType Load(const ThreadIndicesType& indices, const FieldExecObjectType* const field)
  {
    // It is important that we give the VecFromPortalPermute (ValueType) a
    // pointer that will stay around during the time the Vec is valid. Thus, we
    // should make sure that indices is a reference that goes up the stack at
    // least as far as the returned VecFromPortalPermute is used.
    return ValueType(indices.GetIndicesIncidentPointer(), field);
  }
};

static inline VISKORES_EXEC viskores::VecAxisAlignedPointCoordinates<1>
make_VecAxisAlignedPointCoordinates(const viskores::Vec3f& origin,
                                    const viskores::Vec3f& spacing,
                                    const viskores::Vec<viskores::Id, 1>& logicalId)
{
  viskores::Vec3f offsetOrigin(origin[0] +
                                 spacing[0] * static_cast<viskores::FloatDefault>(logicalId[0]),
                               origin[1],
                               origin[2]);
  return viskores::VecAxisAlignedPointCoordinates<1>(offsetOrigin, spacing);
}

static inline VISKORES_EXEC viskores::VecAxisAlignedPointCoordinates<1>
make_VecAxisAlignedPointCoordinates(const viskores::Vec3f& origin,
                                    const viskores::Vec3f& spacing,
                                    viskores::Id logicalId)
{
  return make_VecAxisAlignedPointCoordinates(
    origin, spacing, viskores::Vec<viskores::Id, 1>(logicalId));
}

static inline VISKORES_EXEC viskores::VecAxisAlignedPointCoordinates<2>
make_VecAxisAlignedPointCoordinates(const viskores::Vec3f& origin,
                                    const viskores::Vec3f& spacing,
                                    const viskores::Id2& logicalId)
{
  viskores::Vec3f offsetOrigin(
    origin[0] + spacing[0] * static_cast<viskores::FloatDefault>(logicalId[0]),
    origin[1] + spacing[1] * static_cast<viskores::FloatDefault>(logicalId[1]),
    origin[2]);
  return viskores::VecAxisAlignedPointCoordinates<2>(offsetOrigin, spacing);
}

static inline VISKORES_EXEC viskores::VecAxisAlignedPointCoordinates<3>
make_VecAxisAlignedPointCoordinates(const viskores::Vec3f& origin,
                                    const viskores::Vec3f& spacing,
                                    const viskores::Id3& logicalId)
{
  viskores::Vec3f offsetOrigin(
    origin[0] + spacing[0] * static_cast<viskores::FloatDefault>(logicalId[0]),
    origin[1] + spacing[1] * static_cast<viskores::FloatDefault>(logicalId[1]),
    origin[2] + spacing[2] * static_cast<viskores::FloatDefault>(logicalId[2]));
  return viskores::VecAxisAlignedPointCoordinates<3>(offsetOrigin, spacing);
}

template <viskores::IdComponent NumDimensions, typename ThreadIndicesType>
struct FetchArrayTopologyMapInImplementation<
  viskores::exec::ConnectivityStructured<viskores::TopologyElementTagCell,
                                         viskores::TopologyElementTagPoint,
                                         NumDimensions>,
  viskores::internal::ArrayPortalUniformPointCoordinates,
  ThreadIndicesType>

{
  using ConnectivityType = viskores::exec::ConnectivityStructured<viskores::TopologyElementTagCell,
                                                                  viskores::TopologyElementTagPoint,
                                                                  NumDimensions>;

  using ValueType = viskores::VecAxisAlignedPointCoordinates<NumDimensions>;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  static ValueType Load(const ThreadIndicesType& indices,
                        const viskores::internal::ArrayPortalUniformPointCoordinates& field)
  {
    // This works because the logical cell index is the same as the logical
    // point index of the first point on the cell.
    return viskores::exec::arg::detail::make_VecAxisAlignedPointCoordinates(
      field.GetOrigin(), field.GetSpacing(), indices.GetIndexLogical());
  }
};

template <typename PermutationPortal,
          viskores::IdComponent NumDimensions,
          typename ThreadIndicesType>
struct FetchArrayTopologyMapInImplementation<
  viskores::exec::ConnectivityPermutedVisitCellsWithPoints<
    PermutationPortal,
    viskores::exec::ConnectivityStructured<viskores::TopologyElementTagCell,
                                           viskores::TopologyElementTagPoint,
                                           NumDimensions>>,
  viskores::internal::ArrayPortalUniformPointCoordinates,
  ThreadIndicesType>

{
  using ConnectivityType = viskores::exec::ConnectivityPermutedVisitCellsWithPoints<
    PermutationPortal,
    viskores::exec::ConnectivityStructured<viskores::TopologyElementTagCell,
                                           viskores::TopologyElementTagPoint,
                                           NumDimensions>>;

  using ValueType = viskores::VecAxisAlignedPointCoordinates<NumDimensions>;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  static ValueType Load(const ThreadIndicesType& indices,
                        const viskores::internal::ArrayPortalUniformPointCoordinates& field)
  {
    // This works because the logical cell index is the same as the logical
    // point index of the first point on the cell.

    // we have a flat index but we need 3d uniform coordinates, so we
    // need to take an flat index and convert to logical index
    return viskores::exec::arg::detail::make_VecAxisAlignedPointCoordinates(
      field.GetOrigin(), field.GetSpacing(), indices.GetIndexLogical());
  }
};

} // namespace detail
/// @endcond

template <typename ExecObjectType>
struct Fetch<viskores::exec::arg::FetchTagArrayTopologyMapIn,
             viskores::exec::arg::AspectTagDefault,
             ExecObjectType>
{

  //using ConnectivityType = typename ThreadIndicesType::Connectivity;
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC auto Load(const ThreadIndicesType& indices, const ExecObjectType& field) const
    -> decltype(detail::FetchArrayTopologyMapInImplementation<
                typename ThreadIndicesType::Connectivity,
                ExecObjectType,
                ThreadIndicesType>::Load(indices, field))
  {
    using Implementation =
      detail::FetchArrayTopologyMapInImplementation<typename ThreadIndicesType::Connectivity,
                                                    ExecObjectType,
                                                    ThreadIndicesType>;
    return Implementation::Load(indices, field);
  }

  //Optimized fetch for point arrays when iterating the cells ConnectivityExtrude
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ScatterAndMaskMode>
  VISKORES_EXEC auto Load(
    const viskores::exec::arg::ThreadIndicesTopologyMap<viskores::exec::ConnectivityExtrude,
                                                        ScatterAndMaskMode>& indices,
    const ExecObjectType& portal) -> viskores::Vec<typename ExecObjectType::ValueType, 6>
  {
    // std::cout << "opimized fetch for point values" << std::endl;
    const auto& xgcidx = indices.GetIndicesIncident();
    const viskores::Id offset1 = (xgcidx.Planes[0] * xgcidx.NumberOfPointsPerPlane);
    const viskores::Id offset2 = (xgcidx.Planes[1] * xgcidx.NumberOfPointsPerPlane);

    using ValueType = viskores::Vec<typename ExecObjectType::ValueType, 6>;

    return ValueType(portal.Get(offset1 + xgcidx.PointIds[0][0]),
                     portal.Get(offset1 + xgcidx.PointIds[0][1]),
                     portal.Get(offset1 + xgcidx.PointIds[0][2]),
                     portal.Get(offset2 + xgcidx.PointIds[1][0]),
                     portal.Get(offset2 + xgcidx.PointIds[1][1]),
                     portal.Get(offset2 + xgcidx.PointIds[1][2]));
  }


  template <typename ThreadIndicesType, typename T>
  VISKORES_EXEC void Store(const ThreadIndicesType&, const ExecObjectType&, const T&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_FetchTagArrayTopologyMapIn_h
