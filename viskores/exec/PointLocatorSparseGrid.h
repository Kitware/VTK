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
#ifndef viskores_exec_PointLocatorSparseGrid_h
#define viskores_exec_PointLocatorSparseGrid_h

#include <viskores/cont/CoordinateSystem.h>

#include <viskores/VectorAnalysis.h>

namespace viskores
{
namespace exec
{

/// @brief Structure for locating point.
///
/// Use the `FindNearestNeighbor()` method to identify which cell contains a point in space.
///
/// This class is provided by `viskores::cont::PointLocatorSparseGrid` when passed
/// to a worklet.
class VISKORES_ALWAYS_EXPORT PointLocatorSparseGrid
{
public:
  using CoordPortalType =
    typename viskores::cont::CoordinateSystem::MultiplexerArrayType::ReadPortalType;
  using IdPortalType = typename viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType;


  PointLocatorSparseGrid(const viskores::Vec3f& min,
                         const viskores::Vec3f& max,
                         const viskores::Id3& dims,
                         const CoordPortalType& coords,
                         const IdPortalType& pointIds,
                         const IdPortalType& cellLower,
                         const IdPortalType& cellUpper)
    : Min(min)
    , Dims(dims)
    , Dxdydz((max - Min) / Dims)
    , Coords(coords)
    , PointIds(pointIds)
    , CellLower(cellLower)
    , CellUpper(cellUpper)
  {
  }

  /// @brief Nearest neighbor search using a Uniform Grid
  ///
  /// Parallel search of nearesat neighbor for each point in the `queryPoints` in the set of
  /// `coords`. Returns neareast neighbot in `nearestNeighborIds` and distances to nearest
  /// neighbor in `distances`.
  ///
  /// \param queryPoint Point coordinates to query for nearest neighbor.
  /// \param nearestNeighborId Neareast neighbor in the training dataset for each points in
  ///                            the test set
  /// \param distance2 Squared distance between query points and their nearest neighbors.
  VISKORES_EXEC void FindNearestNeighbor(const viskores::Vec3f& queryPoint,
                                         viskores::Id& nearestNeighborId,
                                         viskores::FloatDefault& distance2) const
  {
    //std::cout << "FindNeareastNeighbor: " << queryPoint << std::endl;
    viskores::Id3 ijk = (queryPoint - this->Min) / this->Dxdydz;
    ijk = viskores::Max(ijk, viskores::Id3(0));
    ijk = viskores::Min(ijk, this->Dims - viskores::Id3(1));

    nearestNeighborId = -1;
    distance2 = viskores::Infinity<viskores::FloatDefault>();

    this->FindInCell(queryPoint, ijk, nearestNeighborId, distance2);

    // TODO: This might stop looking before the absolute nearest neighbor is found.
    viskores::Id maxLevel =
      viskores::Max(viskores::Max(this->Dims[0], this->Dims[1]), this->Dims[2]);
    viskores::Id level;
    for (level = 1; (nearestNeighborId < 0) && (level < maxLevel); ++level)
    {
      this->FindInBox(queryPoint, ijk, level, nearestNeighborId, distance2);
    }

    // Search one more level out. This is still not guaranteed to find the closest point
    // in all cases (past level 2), but it will catch most cases where the closest point
    // is just on the other side of a cell boundary.
    this->FindInBox(queryPoint, ijk, level, nearestNeighborId, distance2);
  }

private:
  viskores::Vec3f Min;
  viskores::Id3 Dims;
  viskores::Vec3f Dxdydz;

  CoordPortalType Coords;

  IdPortalType PointIds;
  IdPortalType CellLower;
  IdPortalType CellUpper;

  VISKORES_EXEC void FindInCell(const viskores::Vec3f& queryPoint,
                                const viskores::Id3& ijk,
                                viskores::Id& nearestNeighborId,
                                viskores::FloatDefault& nearestDistance2) const
  {
    viskores::Id cellId =
      ijk[0] + (ijk[1] * this->Dims[0]) + (ijk[2] * this->Dims[0] * this->Dims[1]);
    viskores::Id lower = this->CellLower.Get(cellId);
    viskores::Id upper = this->CellUpper.Get(cellId);
    for (viskores::Id index = lower; index < upper; index++)
    {
      viskores::Id pointid = this->PointIds.Get(index);
      viskores::Vec3f point = this->Coords.Get(pointid);
      viskores::FloatDefault distance2 = viskores::MagnitudeSquared(point - queryPoint);
      if (distance2 < nearestDistance2)
      {
        nearestNeighborId = pointid;
        nearestDistance2 = distance2;
      }
    }
  }

  VISKORES_EXEC void FindInBox(const viskores::Vec3f& queryPoint,
                               const viskores::Id3& boxCenter,
                               viskores::Id level,
                               viskores::Id& nearestNeighborId,
                               viskores::FloatDefault& nearestDistance2) const
  {
    if ((boxCenter[0] - level) >= 0)
    {
      this->FindInXPlane(queryPoint,
                         boxCenter - viskores::Id3(level, 0, 0),
                         level,
                         nearestNeighborId,
                         nearestDistance2);
    }
    if ((boxCenter[0] + level) < this->Dims[0])
    {
      this->FindInXPlane(queryPoint,
                         boxCenter + viskores::Id3(level, 0, 0),
                         level,
                         nearestNeighborId,
                         nearestDistance2);
    }

    if ((boxCenter[1] - level) >= 0)
    {
      this->FindInYPlane(queryPoint,
                         boxCenter - viskores::Id3(0, level, 0),
                         level,
                         nearestNeighborId,
                         nearestDistance2);
    }
    if ((boxCenter[1] + level) < this->Dims[1])
    {
      this->FindInYPlane(queryPoint,
                         boxCenter + viskores::Id3(0, level, 0),
                         level,
                         nearestNeighborId,
                         nearestDistance2);
    }

    if ((boxCenter[2] - level) >= 0)
    {
      this->FindInZPlane(queryPoint,
                         boxCenter - viskores::Id3(0, 0, level),
                         level,
                         nearestNeighborId,
                         nearestDistance2);
    }
    if ((boxCenter[2] + level) < this->Dims[2])
    {
      this->FindInZPlane(queryPoint,
                         boxCenter + viskores::Id3(0, 0, level),
                         level,
                         nearestNeighborId,
                         nearestDistance2);
    }
  }

  VISKORES_EXEC void FindInPlane(const viskores::Vec3f& queryPoint,
                                 const viskores::Id3& planeCenter,
                                 const viskores::Id3& div,
                                 const viskores::Id3& mod,
                                 const viskores::Id3& origin,
                                 viskores::Id numInPlane,
                                 viskores::Id& nearestNeighborId,
                                 viskores::FloatDefault& nearestDistance2) const
  {
    for (viskores::Id index = 0; index < numInPlane; ++index)
    {
      viskores::Id3 ijk = planeCenter + viskores::Id3(index) / div +
        viskores::Id3(index % mod[0], index % mod[1], index % mod[2]) + origin;
      if ((ijk[0] >= 0) && (ijk[0] < this->Dims[0]) && (ijk[1] >= 0) && (ijk[1] < this->Dims[1]) &&
          (ijk[2] >= 0) && (ijk[2] < this->Dims[2]))
      {
        this->FindInCell(queryPoint, ijk, nearestNeighborId, nearestDistance2);
      }
    }
  }

  VISKORES_EXEC void FindInXPlane(const viskores::Vec3f& queryPoint,
                                  const viskores::Id3& planeCenter,
                                  viskores::Id level,
                                  viskores::Id& nearestNeighborId,
                                  viskores::FloatDefault& nearestDistance2) const
  {
    viskores::Id yWidth = (2 * level) + 1;
    viskores::Id zWidth = (2 * level) + 1;
    viskores::Id3 div = { yWidth * zWidth, yWidth * zWidth, yWidth };
    viskores::Id3 mod = { 1, yWidth, 1 };
    viskores::Id3 origin = { 0, -level, -level };
    viskores::Id numInPlane = yWidth * zWidth;
    this->FindInPlane(
      queryPoint, planeCenter, div, mod, origin, numInPlane, nearestNeighborId, nearestDistance2);
  }

  VISKORES_EXEC void FindInYPlane(const viskores::Vec3f& queryPoint,
                                  viskores::Id3 planeCenter,
                                  viskores::Id level,
                                  viskores::Id& nearestNeighborId,
                                  viskores::FloatDefault& nearestDistance2) const
  {
    viskores::Id xWidth = (2 * level) - 1;
    viskores::Id zWidth = (2 * level) + 1;
    viskores::Id3 div = { xWidth * zWidth, xWidth * zWidth, xWidth };
    viskores::Id3 mod = { xWidth, 1, 1 };
    viskores::Id3 origin = { -level + 1, 0, -level };
    viskores::Id numInPlane = xWidth * zWidth;
    this->FindInPlane(
      queryPoint, planeCenter, div, mod, origin, numInPlane, nearestNeighborId, nearestDistance2);
  }

  VISKORES_EXEC void FindInZPlane(const viskores::Vec3f& queryPoint,
                                  viskores::Id3 planeCenter,
                                  viskores::Id level,
                                  viskores::Id& nearestNeighborId,
                                  viskores::FloatDefault& nearestDistance2) const
  {
    viskores::Id xWidth = (2 * level) - 1;
    viskores::Id yWidth = (2 * level) - 1;
    viskores::Id3 div = { xWidth * yWidth, xWidth, xWidth * yWidth };
    viskores::Id3 mod = { xWidth, 1, 1 };
    viskores::Id3 origin = { -level + 1, -level + 1, 0 };
    viskores::Id numInPlane = xWidth * yWidth;
    this->FindInPlane(
      queryPoint, planeCenter, div, mod, origin, numInPlane, nearestNeighborId, nearestDistance2);
  }
};

} // viskores::exec
} // viskores

#endif // viskores_exec_PointLocatorSparseGrid_h
