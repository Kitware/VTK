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
#ifndef viskores_exec_celllocatorrectilineargrid_h
#define viskores_exec_celllocatorrectilineargrid_h

#include <viskores/Bounds.h>
#include <viskores/TopologyElementTag.h>
#include <viskores/Types.h>
#include <viskores/VecFromPortalPermute.h>

#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/CellSetStructured.h>

#include <viskores/exec/CellInside.h>
#include <viskores/exec/ConnectivityStructured.h>
#include <viskores/exec/ParametricCoordinates.h>

namespace viskores
{

namespace exec
{

/// @brief Structure for locating cells.
///
/// Use the `FindCell()` method to identify which cell contains a point in space.
/// The `FindCell()` method optionally takes a `LastCell` object, which is a
/// structure nested in this class. The `LastCell` object can help speed locating
/// cells for successive finds at nearby points.
///
/// This class is provided by `viskores::cont::CellLocatorRectilinearGrid`
/// when passed to a worklet.
class VISKORES_ALWAYS_EXPORT CellLocatorRectilinearGrid
{
private:
  using AxisHandle = viskores::cont::ArrayHandle<viskores::FloatDefault>;
  using RectilinearType =
    viskores::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
  using AxisPortalType = typename AxisHandle::ReadPortalType;
  using RectilinearPortalType = typename RectilinearType::ReadPortalType;

  // NOLINTNEXTLINE(performance-move-const-arg)
  VISKORES_CONT static viskores::Id3&& ToId3(viskores::Id3&& src) { return std::move(src); }
  VISKORES_CONT static viskores::Id3 ToId3(viskores::Id2&& src)
  {
    return viskores::Id3(src[0], src[1], 1);
  }
  VISKORES_CONT static viskores::Id3 ToId3(viskores::Id&& src) { return viskores::Id3(src, 1, 1); }

public:
  /// @copydoc viskores::exec::CellLocatorUniformGrid::LastCell
  struct LastCell
  {
  };

  template <viskores::IdComponent dimensions>
  VISKORES_CONT CellLocatorRectilinearGrid(
    const viskores::Id planeSize,
    const viskores::Id rowSize,
    const viskores::cont::CellSetStructured<dimensions>& cellSet,
    const RectilinearType& coords,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
    : PlaneSize(planeSize)
    , RowSize(rowSize)
    , PointDimensions(ToId3(cellSet.GetPointDimensions()))
    , Dimensions(dimensions)
  {
    auto coordsContPortal = coords.ReadPortal();
    RectilinearPortalType coordsExecPortal = coords.PrepareForInput(device, token);
    this->AxisPortals[0] = coordsExecPortal.GetFirstPortal();
    this->MinPoint[0] = coordsContPortal.GetFirstPortal().Get(0);
    this->MaxPoint[0] = coordsContPortal.GetFirstPortal().Get(this->PointDimensions[0] - 1);

    this->AxisPortals[1] = coordsExecPortal.GetSecondPortal();
    this->MinPoint[1] = coordsContPortal.GetSecondPortal().Get(0);
    this->MaxPoint[1] = coordsContPortal.GetSecondPortal().Get(this->PointDimensions[1] - 1);
    if (dimensions == 3)
    {
      this->AxisPortals[2] = coordsExecPortal.GetThirdPortal();
      this->MinPoint[2] = coordsContPortal.GetThirdPortal().Get(0);
      this->MaxPoint[2] = coordsContPortal.GetThirdPortal().Get(this->PointDimensions[2] - 1);
    }
  }

  VISKORES_EXEC
  inline bool IsInside(const viskores::Vec3f& point) const
  {
    bool inside = true;
    if (point[0] < this->MinPoint[0] || point[0] > this->MaxPoint[0])
      inside = false;
    if (point[1] < this->MinPoint[1] || point[1] > this->MaxPoint[1])
      inside = false;
    if (this->Dimensions == 3)
    {
      if (point[2] < this->MinPoint[2] || point[2] > this->MaxPoint[2])
        inside = false;
    }
    return inside;
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCell
  VISKORES_EXEC viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                                             viskores::Id& cellId,
                                             viskores::Vec3f& parametric,
                                             LastCell& viskoresNotUsed(lastCell)) const
  {
    return this->FindCell(point, cellId, parametric);
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCell
  VISKORES_EXEC viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                                             viskores::Id& cellId,
                                             viskores::Vec3f& parametric) const
  {
    if (!this->IsInside(point))
    {
      cellId = -1;
      return viskores::ErrorCode::CellNotFound;
    }

    // Get the Cell Id from the point.
    viskores::Id3 logicalCell(0, 0, 0);
    for (viskores::Int32 dim = 0; dim < this->Dimensions; ++dim)
    {
      //
      // When searching for points, we consider the max value of the cell
      // to be apart of the next cell. If the point falls on the boundary of the
      // data set, then it is technically inside a cell. This checks for that case
      //
      if (point[dim] == MaxPoint[dim])
      {
        logicalCell[dim] = this->PointDimensions[dim] - 2;
        parametric[dim] = static_cast<viskores::FloatDefault>(1);
        continue;
      }

      viskores::Id minIndex = 0;
      viskores::Id maxIndex = this->PointDimensions[dim] - 1;
      viskores::FloatDefault minVal;
      viskores::FloatDefault maxVal;
      minVal = this->AxisPortals[dim].Get(minIndex);
      maxVal = this->AxisPortals[dim].Get(maxIndex);
      while (maxIndex > minIndex + 1)
      {
        viskores::Id midIndex = (minIndex + maxIndex) / 2;
        viskores::FloatDefault midVal = this->AxisPortals[dim].Get(midIndex);
        if (point[dim] <= midVal)
        {
          maxIndex = midIndex;
          maxVal = midVal;
        }
        else
        {
          minIndex = midIndex;
          minVal = midVal;
        }
      }
      logicalCell[dim] = minIndex;
      parametric[dim] = (point[dim] - minVal) / (maxVal - minVal);
    }
    // Get the actual cellId, from the logical cell index of the cell
    cellId = logicalCell[2] * this->PlaneSize + logicalCell[1] * this->RowSize + logicalCell[0];

    return viskores::ErrorCode::Success;
  }

private:
  viskores::Id PlaneSize;
  viskores::Id RowSize;

  AxisPortalType AxisPortals[3];
  viskores::Id3 PointDimensions;
  viskores::Vec3f MinPoint;
  viskores::Vec3f MaxPoint;
  viskores::Id Dimensions;
};
} //namespace exec
} //namespace viskores

#endif //viskores_exec_celllocatorrectilineargrid_h
