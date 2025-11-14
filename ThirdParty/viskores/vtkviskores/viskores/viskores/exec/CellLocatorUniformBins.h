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
#ifndef viskores_exec_CellLocatorUniformBins_h
#define viskores_exec_CellLocatorUniformBins_h

#include <viskores/exec/CellInside.h>
#include <viskores/exec/ParametricCoordinates.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/CoordinateSystem.h>

#include <viskores/TopologyElementTag.h>
#include <viskores/VecFromPortalPermute.h>
#include <viskores/VecTraits.h>

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
/// This class is provided by `viskores::cont::CellLocatorBoundingIntervalHierarchy`
/// when passed to a worklet.
template <typename CellStructureType>
class VISKORES_ALWAYS_EXPORT CellLocatorUniformBins
{
  template <typename T>
  using ReadPortal = typename viskores::cont::ArrayHandle<T>::ReadPortalType;

  using CoordsPortalType =
    typename viskores::cont::CoordinateSystem::MultiplexerArrayType::ReadPortalType;

  using CellIdArrayType = viskores::cont::ArrayHandle<viskores::Id>;
  using CellIdOffsetArrayType = viskores::cont::ArrayHandle<viskores::Id>;
  using CellIdReadPortal =
    typename viskores::cont::ArrayHandleGroupVecVariable<CellIdArrayType,
                                                         CellIdOffsetArrayType>::ReadPortalType;

public:
  template <typename CellSetType>
  VISKORES_CONT CellLocatorUniformBins(
    const viskores::Id3& cellDims,
    const viskores::Vec3f& origin,
    const viskores::Vec3f& maxPoint,
    const viskores::Vec3f& invSpacing,
    const viskores::Id3& maxCellIds,
    const viskores::cont::ArrayHandleGroupVecVariable<CellIdArrayType, CellIdOffsetArrayType>&
      cellIds,
    const CellSetType& cellSet,
    const viskores::cont::CoordinateSystem& coords,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
    : CellDims(cellDims)
    , Origin(origin)
    , MaxPoint(maxPoint)
    , InvSpacing(invSpacing)
    , MaxCellIds(maxCellIds)
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
    viskores::Id BinIdx = -1;
  };

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCell
  VISKORES_EXEC viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                                             viskores::Id& cellId,
                                             viskores::Vec3f& pCoords) const
  {
    LastCell lastCell;
    return this->FindCell(point, cellId, pCoords, lastCell);
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCell
  VISKORES_EXEC viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                                             viskores::Id& cellId,
                                             viskores::Vec3f& pCoords,
                                             LastCell& lastCell) const
  {
    viskores::Id binIdx = this->FindBinIdx(point);
    if (binIdx == -1)
    {
      lastCell.CellId = -1;
      lastCell.BinIdx = -1;
      cellId = -1;
      return viskores::ErrorCode::CellNotFound;
    }
    //See if the point is still in the same bin.
    else if (binIdx == lastCell.BinIdx && this->LastCellValid(lastCell))
    {
      viskores::Vec3f pc;
      //Check the last cell first.
      if (this->PointInCell(point, lastCell.CellId, pc))
      {
        pCoords = pc;
        cellId = lastCell.CellId;
        return viskores::ErrorCode::Success;
      }
      //Otherwise, check cells in the bin, but skip lastCell.CellId
      else if (this->PointInBin(point, lastCell.BinIdx, cellId, pc, lastCell.CellId))
      {
        pCoords = pc;
        return viskores::ErrorCode::Success;
      }
    }

    //if cell still not found, drop to the general find cell below.
    //LastCell not initialized, or not in the same bin: do a full test.
    //Since already computed the binIdx, re-use it.
    viskores::Vec<viskores::Id, 1> cellIdVec = { -1 };
    viskores::Vec<viskores::Vec3f, 1> pCoordsVec;
    auto nCells = this->IterateBin(point, IterateMode::FindOne, cellIdVec, pCoordsVec, binIdx);
    if (nCells == 1)
    {
      cellId = cellIdVec[0];
      pCoords = pCoordsVec[0];
      lastCell.BinIdx = binIdx;
      lastCell.CellId = cellId;
      return viskores::ErrorCode::Success;
    }

    cellId = -1;
    return viskores::ErrorCode::CellNotFound;
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::CountAllCells
  VISKORES_EXEC viskores::IdComponent CountAllCells(const viskores::Vec3f& point) const
  {
    viskores::Id binIdx = this->FindBinIdx(point);
    if (binIdx == -1)
      return 0;

    viskores::Vec<viskores::Vec3f, 1> pcVec;
    viskores::Vec<viskores::Id, 1> cellIdVec = { -1 };
    return this->IterateBin(point, IterateMode::CountAll, cellIdVec, pcVec, binIdx);
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindAllCells
  template <typename CellIdsType, typename ParametricCoordsVecType>
  VISKORES_EXEC viskores::ErrorCode FindAllCells(const viskores::Vec3f& point,
                                                 CellIdsType& cellIdVec,
                                                 ParametricCoordsVecType& pCoordsVec) const
  {
    viskores::IdComponent n = cellIdVec.GetNumberOfComponents();
    if (n == 0)
      return viskores::ErrorCode::Success;

    for (viskores::IdComponent i = 0; i < n; i++)
      cellIdVec[i] = -1;

    VISKORES_ASSERT(n == pCoordsVec.GetNumberOfComponents());

    viskores::Id binIdx = this->FindBinIdx(point);
    if (binIdx == -1)
      return viskores::ErrorCode::CellNotFound;

    viskores::IdComponent cnt =
      this->IterateBin(point, IterateMode::FindAll, cellIdVec, pCoordsVec, binIdx);
    if (cnt < 0)
      return viskores::ErrorCode::InvalidNumberOfIndices;
    else if (cnt == 0)
      return viskores::ErrorCode::CellNotFound;
    else
      return viskores::ErrorCode::Success;
  }

  VISKORES_DEPRECATED(1.6, "Locators are no longer pointers. Use . operator.")
  VISKORES_EXEC CellLocatorUniformBins* operator->() { return this; }
  VISKORES_DEPRECATED(1.6, "Locators are no longer pointers. Use . operator.")
  VISKORES_EXEC const CellLocatorUniformBins* operator->() const { return this; }

private:
  enum class IterateMode
  {
    FindOne,
    CountAll,
    FindAll
  };

  VISKORES_EXEC viskores::Id FindBinIdx(const viskores::Vec3f& point) const
  {
    if (!this->IsInside(point))
      return -1;

    viskores::Vec3f temp;
    temp = point - this->Origin;
    temp = temp * this->InvSpacing;

    //make sure that if we border the upper edge, we sample the correct cell
    viskores::Id3 logicalCell = viskores::Min(viskores::Id3(temp), this->MaxCellIds);

    viskores::Id binIdx =
      (logicalCell[2] * this->CellDims[1] + logicalCell[1]) * this->CellDims[0] + logicalCell[0];

    return binIdx;
  }

  VISKORES_EXEC bool LastCellValid(const LastCell& lastCell) const
  {
    return lastCell.BinIdx >= 0 && lastCell.BinIdx < this->CellIds.GetNumberOfValues() &&
      lastCell.CellId >= 0 && lastCell.CellId < this->CellSet.GetNumberOfElements();
  }

  VISKORES_EXEC bool IsInside(const viskores::Vec3f& point) const
  {
    if (point[0] < this->Origin[0] || point[0] > this->MaxPoint[0])
      return false;
    if (point[1] < this->Origin[1] || point[1] > this->MaxPoint[1])
      return false;
    if (point[2] < this->Origin[2] || point[2] > this->MaxPoint[2])
      return false;

    return true;
  }

  template <typename CellIdVecType, typename ParametricCoordsVecType>
  VISKORES_EXEC viskores::IdComponent IterateBin(const viskores::Vec3f& point,
                                                 const IterateMode& mode,
                                                 CellIdVecType& cellIdVec,
                                                 ParametricCoordsVecType& pCoordsVec,
                                                 viskores::Id binIdx) const
  {
    viskores::IdComponent n = cellIdVec.GetNumberOfComponents();
    VISKORES_ASSERT(pCoordsVec.GetNumberOfComponents() == n);

    auto binIds = this->CellIds.Get(binIdx);

    viskores::Vec3f pc;
    viskores::IdComponent cellCount = 0;
    for (viskores::IdComponent i = 0; i < binIds.GetNumberOfComponents(); i++)
    {
      viskores::Id cid = binIds[i];
      if (this->PointInCell(point, cid, pc))
      {
        if (mode != IterateMode::CountAll)
        {
          cellIdVec[cellCount] = cid;
          pCoordsVec[cellCount] = pc;
        }
        cellCount++;
        if (mode == IterateMode::FindOne || (mode == IterateMode::FindAll && cellCount == n))
          break;
      }
    }

    return cellCount;
  }

  template <typename PointsVecType>
  VISKORES_EXEC viskores::Bounds ComputeCellBounds(const PointsVecType& points) const
  {
    auto numPoints = viskores::VecTraits<PointsVecType>::GetNumberOfComponents(points);

    viskores::Bounds bounds;
    for (viskores::IdComponent i = 0; i < numPoints; ++i)
      bounds.Include(points[i]);

    return bounds;
  }

  // TODO: This function may return false positives for non 3D cells as the
  // tests are done on the projection of the point on the cell. Extra checks
  // should be added to test if the point actually falls on the cell.
  template <typename CellShapeTag, typename CoordsType>
  VISKORES_EXEC viskores::ErrorCode PointInsideCell(viskores::Vec3f point,
                                                    CellShapeTag cellShape,
                                                    CoordsType cellPoints,
                                                    viskores::Vec3f& pCoords,
                                                    bool& inside) const
  {
    auto bounds = this->ComputeCellBounds(cellPoints);
    if (bounds.Contains(point))
    {
      VISKORES_RETURN_ON_ERROR(viskores::exec::WorldCoordinatesToParametricCoordinates(
        cellPoints, point, cellShape, pCoords));
      inside = viskores::exec::CellInside(pCoords, cellShape);
    }
    else
    {
      inside = false;
    }
    // Return success error code even point is not inside this cell
    return viskores::ErrorCode::Success;
  }

  VISKORES_EXEC
  bool PointInBin(const viskores::Vec3f& point,
                  const viskores::Id& binIdx,
                  viskores::Id& cellId,
                  viskores::Vec3f& pCoords,
                  const viskores::Id& skipCellId = -1) const
  {
    auto binIds = this->CellIds.Get(binIdx);

    viskores::Vec3f pc;
    for (viskores::IdComponent i = 0; i < binIds.GetNumberOfComponents(); i++)
    {
      viskores::Id cid = binIds[i];
      if (cid != skipCellId && this->PointInCell(point, cid, pc))
      {
        cellId = cid;
        pCoords = pc;
        return true;
      }
    }

    return false;
  }

  VISKORES_EXEC
  bool PointInCell(const viskores::Vec3f& point,
                   const viskores::Id& cid,
                   viskores::Vec3f& pCoords) const
  {
    auto indices = this->CellSet.GetIndices(cid);
    auto pts = viskores::make_VecFromPortalPermute(&indices, this->Coords);
    viskores::Vec3f pc;
    bool inside;
    auto status = this->PointInsideCell(point, this->CellSet.GetCellShape(cid), pts, pc, inside);
    if (status == viskores::ErrorCode::Success && inside)
    {
      pCoords = pc;
      return true;
    }

    return false;
  }

  viskores::Id3 CellDims;
  viskores::Vec3f Origin;
  viskores::Vec3f MaxPoint;
  viskores::Vec3f InvSpacing;
  viskores::Id3 MaxCellIds;

  CellIdReadPortal CellIds;

  CellStructureType CellSet;
  CoordsPortalType Coords;
};
}
} // viskores::exec

#endif //viskores_exec_CellLocatorUniformBins_h
