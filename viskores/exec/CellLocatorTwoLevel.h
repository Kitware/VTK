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
#ifndef viskores_exec_CellLocatorTwoLevel_h
#define viskores_exec_CellLocatorTwoLevel_h

#include <viskores/exec/CellInside.h>
#include <viskores/exec/ParametricCoordinates.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CoordinateSystem.h>

#include <viskores/Math.h>
#include <viskores/TopologyElementTag.h>
#include <viskores/Types.h>
#include <viskores/VecFromPortalPermute.h>
#include <viskores/VecTraits.h>

namespace viskores
{
namespace internal
{
namespace cl_uniform_bins
{

using DimensionType = viskores::Int16;
using DimVec3 = viskores::Vec<DimensionType, 3>;
using FloatVec3 = viskores::Vec3f;

struct Grid
{
  DimVec3 Dimensions;
  // Bug in CUDA 9.2 where having this gap for alignment was for some reason setting garbage
  // in a union with other cell locators (or perhaps not properly copying data). This appears
  // to be fixed by CUDA 10.2.
  DimensionType Padding;
  FloatVec3 Origin;
  FloatVec3 BinSize;
};

struct Bounds
{
  FloatVec3 Min;
  FloatVec3 Max;
};

VISKORES_EXEC inline viskores::Id ComputeFlatIndex(const DimVec3& idx, const DimVec3 dim)
{
  return idx[0] + (dim[0] * (idx[1] + (dim[1] * idx[2])));
}

VISKORES_EXEC inline Grid ComputeLeafGrid(const DimVec3& idx,
                                          const DimVec3& dim,
                                          const Grid& l1Grid)
{
  return { dim,
           0,
           l1Grid.Origin + (static_cast<FloatVec3>(idx) * l1Grid.BinSize),
           l1Grid.BinSize / static_cast<FloatVec3>(dim) };
}

template <typename PointsVecType>
VISKORES_EXEC inline Bounds ComputeCellBounds(const PointsVecType& points)
{
  using CoordsType = typename viskores::VecTraits<PointsVecType>::ComponentType;
  auto numPoints = viskores::VecTraits<PointsVecType>::GetNumberOfComponents(points);

  CoordsType minp = points[0], maxp = points[0];
  for (viskores::IdComponent i = 1; i < numPoints; ++i)
  {
    minp = viskores::Min(minp, points[i]);
    maxp = viskores::Max(maxp, points[i]);
  }

  return { FloatVec3(minp), FloatVec3(maxp) };
}
}
}
} // viskores::internal::cl_uniform_bins

namespace viskores
{
namespace exec
{

//--------------------------------------------------------------------

/// @brief Structure for locating cells.
///
/// Use the `FindCell()` method to identify which cell contains a point in space.
/// The `FindCell()` method optionally takes a `LastCell` object, which is a
/// structure nested in this class. The `LastCell` object can help speed locating
/// cells for successive finds at nearby points.
///
/// This class is provided by `viskores::cont::CellLocatorTwoLevel`
/// when passed to a worklet.
template <typename CellStructureType>
class VISKORES_ALWAYS_EXPORT CellLocatorTwoLevel
{
private:
  using DimVec3 = viskores::internal::cl_uniform_bins::DimVec3;
  using FloatVec3 = viskores::internal::cl_uniform_bins::FloatVec3;

  template <typename T>
  using ReadPortal = typename viskores::cont::ArrayHandle<T>::ReadPortalType;

  using CoordsPortalType =
    typename viskores::cont::CoordinateSystem::MultiplexerArrayType::ReadPortalType;

  // TODO: This function may return false positives for non 3D cells as the
  // tests are done on the projection of the point on the cell. Extra checks
  // should be added to test if the point actually falls on the cell.
  template <typename CellShapeTag, typename CoordsType>
  VISKORES_EXEC static viskores::ErrorCode PointInsideCell(FloatVec3 point,
                                                           CellShapeTag cellShape,
                                                           CoordsType cellPoints,
                                                           FloatVec3& parametricCoordinates,
                                                           bool& inside)
  {
    auto bounds = viskores::internal::cl_uniform_bins::ComputeCellBounds(cellPoints);
    if (point[0] >= bounds.Min[0] && point[0] <= bounds.Max[0] && point[1] >= bounds.Min[1] &&
        point[1] <= bounds.Max[1] && point[2] >= bounds.Min[2] && point[2] <= bounds.Max[2])
    {
      VISKORES_RETURN_ON_ERROR(viskores::exec::WorldCoordinatesToParametricCoordinates(
        cellPoints, point, cellShape, parametricCoordinates));
      inside = viskores::exec::CellInside(parametricCoordinates, cellShape);
    }
    else
    {
      inside = false;
    }
    // Return success error code even point is not inside this cell
    return viskores::ErrorCode::Success;
  }

public:
  template <typename CellSetType>
  VISKORES_CONT CellLocatorTwoLevel(const viskores::internal::cl_uniform_bins::Grid& topLevelGrid,
                                    const viskores::cont::ArrayHandle<DimVec3>& leafDimensions,
                                    const viskores::cont::ArrayHandle<viskores::Id>& leafStartIndex,
                                    const viskores::cont::ArrayHandle<viskores::Id>& cellStartIndex,
                                    const viskores::cont::ArrayHandle<viskores::Id>& cellCount,
                                    const viskores::cont::ArrayHandle<viskores::Id>& cellIds,
                                    const CellSetType& cellSet,
                                    const viskores::cont::CoordinateSystem& coords,
                                    viskores::cont::DeviceAdapterId device,
                                    viskores::cont::Token& token)
    : TopLevel(topLevelGrid)
    , LeafDimensions(leafDimensions.PrepareForInput(device, token))
    , LeafStartIndex(leafStartIndex.PrepareForInput(device, token))
    , CellStartIndex(cellStartIndex.PrepareForInput(device, token))
    , CellCount(cellCount.PrepareForInput(device, token))
    , CellIds(cellIds.PrepareForInput(device, token))
    , CellSet(cellSet.PrepareForInput(device,
                                      viskores::TopologyElementTagCell{},
                                      viskores::TopologyElementTagPoint{},
                                      token))
    , Coords(coords.GetDataAsMultiplexer().PrepareForInput(device, token))
  {
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::LastCell
  struct LastCell
  {
    viskores::Id CellId = -1;
    viskores::Id LeafIdx = -1;
  };

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCell
  VISKORES_EXEC
  viskores::ErrorCode FindCell(const FloatVec3& point,
                               viskores::Id& cellId,
                               FloatVec3& parametric) const
  {
    LastCell lastCell;
    return this->FindCellImpl(point, cellId, parametric, lastCell);
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCell
  VISKORES_EXEC
  viskores::ErrorCode FindCell(const FloatVec3& point,
                               viskores::Id& cellId,
                               FloatVec3& parametric,
                               LastCell& lastCell) const
  {
    viskores::Vec3f pc;
    //See if point is inside the last cell.
    if ((lastCell.CellId >= 0) && (lastCell.CellId < this->CellSet.GetNumberOfElements()) &&
        this->PointInCell(point, lastCell.CellId, pc) == viskores::ErrorCode::Success)
    {
      parametric = pc;
      cellId = lastCell.CellId;
      return viskores::ErrorCode::Success;
    }

    //See if it's in the last leaf.
    if ((lastCell.LeafIdx >= 0) && (lastCell.LeafIdx < this->CellCount.GetNumberOfValues()) &&
        this->PointInLeaf(point, lastCell.LeafIdx, cellId, pc) == viskores::ErrorCode::Success)
    {
      parametric = pc;
      lastCell.CellId = cellId;
      return viskores::ErrorCode::Success;
    }

    //Call the full point search.
    return this->FindCellImpl(point, cellId, parametric, lastCell);
  }

private:
  VISKORES_EXEC
  viskores::ErrorCode PointInCell(const viskores::Vec3f& point,
                                  const viskores::Id& cid,
                                  viskores::Vec3f& parametric) const
  {
    auto indices = this->CellSet.GetIndices(cid);
    auto pts = viskores::make_VecFromPortalPermute(&indices, this->Coords);
    viskores::Vec3f pc;
    bool inside;
    auto status = PointInsideCell(point, this->CellSet.GetCellShape(cid), pts, pc, inside);
    if (status == viskores::ErrorCode::Success && inside)
    {
      parametric = pc;
      return viskores::ErrorCode::Success;
    }

    return viskores::ErrorCode::CellNotFound;
  }

  VISKORES_EXEC
  viskores::ErrorCode PointInLeaf(const FloatVec3& point,
                                  const viskores::Id& leafIdx,
                                  viskores::Id& cellId,
                                  FloatVec3& parametric) const
  {
    viskores::Id start = this->CellStartIndex.Get(leafIdx);
    viskores::Id end = start + this->CellCount.Get(leafIdx);

    for (viskores::Id i = start; i < end; ++i)
    {
      viskores::Vec3f pc;

      viskores::Id cid = this->CellIds.Get(i);
      if (this->PointInCell(point, cid, pc) == viskores::ErrorCode::Success)
      {
        cellId = cid;
        parametric = pc;
        return viskores::ErrorCode::Success;
      }
    }

    return viskores::ErrorCode::CellNotFound;
  }


  VISKORES_EXEC
  viskores::ErrorCode FindCellImpl(const FloatVec3& point,
                                   viskores::Id& cellId,
                                   FloatVec3& parametric,
                                   LastCell& lastCell) const
  {
    using namespace viskores::internal::cl_uniform_bins;

    cellId = -1;
    lastCell.CellId = -1;
    lastCell.LeafIdx = -1;

    DimVec3 binId3 = static_cast<DimVec3>((point - this->TopLevel.Origin) / this->TopLevel.BinSize);
    if (binId3[0] >= 0 && binId3[0] < this->TopLevel.Dimensions[0] && binId3[1] >= 0 &&
        binId3[1] < this->TopLevel.Dimensions[1] && binId3[2] >= 0 &&
        binId3[2] < this->TopLevel.Dimensions[2])
    {
      viskores::Id binId = ComputeFlatIndex(binId3, this->TopLevel.Dimensions);

      auto ldim = this->LeafDimensions.Get(binId);
      if (!ldim[0] || !ldim[1] || !ldim[2])
      {
        return viskores::ErrorCode::CellNotFound;
      }

      auto leafGrid = ComputeLeafGrid(binId3, ldim, this->TopLevel);

      DimVec3 leafId3 = static_cast<DimVec3>((point - leafGrid.Origin) / leafGrid.BinSize);
      // precision issues may cause leafId3 to be out of range so clamp it
      leafId3 = viskores::Max(DimVec3(0), viskores::Min(ldim - DimVec3(1), leafId3));

      viskores::Id leafStart = this->LeafStartIndex.Get(binId);
      viskores::Id leafIdx = leafStart + ComputeFlatIndex(leafId3, leafGrid.Dimensions);

      if (this->PointInLeaf(point, leafIdx, cellId, parametric) == viskores::ErrorCode::Success)
      {
        lastCell.CellId = cellId;
        lastCell.LeafIdx = leafIdx;
        return viskores::ErrorCode::Success;
      }
    }

    return viskores::ErrorCode::CellNotFound;
  }

  viskores::internal::cl_uniform_bins::Grid TopLevel;

  ReadPortal<DimVec3> LeafDimensions;
  ReadPortal<viskores::Id> LeafStartIndex;

  ReadPortal<viskores::Id> CellStartIndex;
  ReadPortal<viskores::Id> CellCount;
  ReadPortal<viskores::Id> CellIds;

  CellStructureType CellSet;
  CoordsPortalType Coords;
};
}
} // viskores::exec

#endif //viskores_exec_CellLocatorTwoLevel_h
