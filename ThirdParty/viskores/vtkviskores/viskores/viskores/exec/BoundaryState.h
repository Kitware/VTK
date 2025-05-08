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
#ifndef viskores_exec_BoundaryState_h
#define viskores_exec_BoundaryState_h

#include <viskores/Assert.h>
#include <viskores/Math.h>

namespace viskores
{
namespace exec
{

/// @brief Provides a neighborhood's placement with respect to the mesh's boundary.
///
/// `BoundaryState` provides functionality for `viskores::worklet::WorkletPointNeighborhood` algorithms
/// to determine if they are operating on a point near the boundary. It allows you to query about
/// overlaps of the neighborhood and the mesh boundary. It also helps convert local neighborhood
/// ids to the corresponding location in the mesh.
///
/// This class is typically constructed using the `Boundary` tag in an `ExecutionSignature`.
/// There is little reason to construct this in user code.
///
struct BoundaryState
{
  VISKORES_EXEC
  BoundaryState(const viskores::Id3& ijk, const viskores::Id3& pdims)
    : IJK(ijk)
    , PointDimensions(pdims)
  {
  }

  /// Returns the center index of the neighborhood. This is typically the position of the
  /// invocation of the worklet given this boundary condition.
  ///
  VISKORES_EXEC const viskores::Id3& GetCenterIndex() const { return this->IJK; }

  /// Returns true if a neighborhood of the given radius is contained within the bounds of the cell
  /// set in the X, Y, or Z direction. Returns false if the neighborhood extends outside of the
  /// boundary of the data in the X, Y, or Z direction.
  ///
  /// The radius defines the size of the neighborhood in terms of how far away it extends from the
  /// center. So if there is a radius of 1, the neighborhood extends 1 unit away from the center in
  /// each direction and is 3x3x3. If there is a radius of 2, the neighborhood extends 2 units for
  /// a size of 5x5x5.
  ///
  VISKORES_EXEC bool IsRadiusInXBoundary(viskores::IdComponent radius) const
  {
    VISKORES_ASSERT(radius >= 0);
    return (((this->IJK[0] - radius) >= 0) && ((this->IJK[0] + radius) < this->PointDimensions[0]));
  }
  /// @copydoc IsRadiusInXBoundary
  VISKORES_EXEC bool IsRadiusInYBoundary(viskores::IdComponent radius) const
  {
    VISKORES_ASSERT(radius >= 0);
    return (((this->IJK[1] - radius) >= 0) && ((this->IJK[1] + radius) < this->PointDimensions[1]));
  }
  /// @copydoc IsRadiusInXBoundary
  VISKORES_EXEC bool IsRadiusInZBoundary(viskores::IdComponent radius) const
  {
    VISKORES_ASSERT(radius >= 0);
    return (((this->IJK[2] - radius) >= 0) && ((this->IJK[2] + radius) < this->PointDimensions[2]));
  }

  /// Returns true if a neighborhood of the given radius is contained within the bounds
  /// of the cell set. Returns false if the neighborhood extends outside of the boundary of the
  /// data.
  ///
  /// The radius defines the size of the neighborhood in terms of how far away it extends from the
  /// center. So if there is a radius of 1, the neighborhood extends 1 unit away from the center in
  /// each direction and is 3x3x3. If there is a radius of 2, the neighborhood extends 2 units for
  /// a size of 5x5x5.
  ///
  VISKORES_EXEC bool IsRadiusInBoundary(viskores::IdComponent radius) const
  {
    return this->IsRadiusInXBoundary(radius) && this->IsRadiusInYBoundary(radius) &&
      this->IsRadiusInZBoundary(radius);
  }

  /// Returns true if the neighbor at the specified @a offset is contained
  /// within the bounds of the cell set in the X, Y, or Z direction. Returns
  /// false if the neighbor falls outside of the boundary of the data in the X,
  /// Y, or Z direction.
  ///
  VISKORES_EXEC bool IsNeighborInXBoundary(viskores::IdComponent offset) const
  {
    return (((this->IJK[0] + offset) >= 0) && ((this->IJK[0] + offset) < this->PointDimensions[0]));
  }
  /// @copydoc IsNeighborInXBoundary
  VISKORES_EXEC bool IsNeighborInYBoundary(viskores::IdComponent offset) const
  {
    return (((this->IJK[1] + offset) >= 0) && ((this->IJK[1] + offset) < this->PointDimensions[1]));
  }
  /// @copydoc IsNeighborInXBoundary
  VISKORES_EXEC bool IsNeighborInZBoundary(viskores::IdComponent offset) const
  {
    return (((this->IJK[2] + offset) >= 0) && ((this->IJK[2] + offset) < this->PointDimensions[2]));
  }

  /// Returns true if the neighbor at the specified offset vector is contained
  /// within the bounds of the cell set. Returns false if the neighbor falls
  /// outside of the boundary of the data.
  ///
  VISKORES_EXEC bool IsNeighborInBoundary(const viskores::IdComponent3& neighbor) const
  {
    return this->IsNeighborInXBoundary(neighbor[0]) && this->IsNeighborInYBoundary(neighbor[1]) &&
      this->IsNeighborInZBoundary(neighbor[2]);
  }

  /// @brief Returns the minimum neighborhood indices that are within the bounds of the data.
  ///
  /// Given a radius for the neighborhood, returns a `viskores::IdComponent3` for the "lower left"
  /// (minimum) index. If the visited point is in the middle of the mesh, the returned triplet
  /// is the negative radius for all components. But if the visited point is near the mesh
  /// boundary, then the minimum index will be clipped.
  ///
  /// For example, if the visited point is at [5,5,5] and `MinNeighborIndices(2)` is called,
  /// then [-2,-2,-2] is returned. However, if the visited point is at [0,1,2] and
  /// `MinNeighborIndices(2)` is called, then [0,-1,-2] is returned.
  VISKORES_EXEC viskores::IdComponent3 MinNeighborIndices(viskores::IdComponent radius) const
  {
    VISKORES_ASSERT(radius >= 0);
    viskores::IdComponent3 minIndices;

    for (viskores::IdComponent component = 0; component < 3; ++component)
    {
      if (this->IJK[component] >= radius)
      {
        minIndices[component] = -radius;
      }
      else
      {
        minIndices[component] = static_cast<viskores::IdComponent>(-this->IJK[component]);
      }
    }

    return minIndices;
  }

  /// @brief Returns the minimum neighborhood indices that are within the bounds of the data.
  ///
  /// Given a radius for the neighborhood, returns a `viskores::IdComponent3` for the "upper right"
  /// (maximum) index. If the visited point is in the middle of the mesh, the returned triplet
  /// is the positive radius for all components. But if the visited point is near the mesh
  /// boundary, then the maximum index will be clipped.
  ///
  /// For example, if the visited point is at [5,5,5] in a 10 by 10 by 10 mesh and
  /// `MaxNeighborIndices(2)` is called, then [2,2,2] is returned. However, if the visited point
  /// is at [7, 8, 9] in the same mesh and `MaxNeighborIndices(2)` is called, then [2, 1, 0]
  /// is returned.
  VISKORES_EXEC viskores::IdComponent3 MaxNeighborIndices(viskores::IdComponent radius) const
  {
    VISKORES_ASSERT(radius >= 0);
    viskores::IdComponent3 maxIndices;

    for (viskores::IdComponent component = 0; component < 3; ++component)
    {
      if ((this->PointDimensions[component] - this->IJK[component] - 1) >= radius)
      {
        maxIndices[component] = radius;
      }
      else
      {
        maxIndices[component] = static_cast<viskores::IdComponent>(
          this->PointDimensions[component] - this->IJK[component] - 1);
      }
    }

    return maxIndices;
  }

  /// Takes a local neighborhood index (in the ranges of -neighborhood size to neighborhood size)
  /// and returns the ijk of the equivalent point in the full data set. If the given value is out
  /// of range, the value is clamped to the nearest boundary. For example, if given a neighbor
  /// index that is past the minimum x range of the data, the index at the minimum x boundary is
  /// returned.
  ///
  VISKORES_EXEC viskores::Id3 NeighborIndexToFullIndexClamp(
    const viskores::IdComponent3& neighbor) const
  {
    viskores::Id3 fullIndex = this->IJK + neighbor;

    return viskores::Max(viskores::Id3(0),
                         viskores::Min(this->PointDimensions - viskores::Id3(1), fullIndex));
  }

  /// @copydoc NeighborIndexToFullIndexClamp
  VISKORES_EXEC viskores::Id3 NeighborIndexToFullIndexClamp(viskores::IdComponent neighborI,
                                                            viskores::IdComponent neighborJ,
                                                            viskores::IdComponent neighborK) const
  {
    return this->NeighborIndexToFullIndexClamp(viskores::make_Vec(neighborI, neighborJ, neighborK));
  }

  /// Takes a local neighborhood index (in the ranges of -neighborhood size to neighborhood size)
  /// and returns the ijk of the equivalent point in the full data set. If the given value is out
  /// of range, the returned value is undefined.
  ///
  VISKORES_EXEC viskores::Id3 NeighborIndexToFullIndex(const viskores::IdComponent3& neighbor) const
  {
    return this->IJK + neighbor;
  }

  /// @copydoc NeighborIndexToFullIndex
  VISKORES_EXEC viskores::Id3 NeighborIndexToFullIndex(viskores::IdComponent neighborI,
                                                       viskores::IdComponent neighborJ,
                                                       viskores::IdComponent neighborK) const
  {
    return this->NeighborIndexToFullIndex(viskores::make_Vec(neighborI, neighborJ, neighborK));
  }

  /// Takes a local neighborhood index (in the ranges of -neighborhood size to
  /// neighborhood size), clamps it to the dataset bounds, and returns a new
  /// neighborhood index. For example, if given a neighbor index that is past
  /// the minimum x range of the data, the neighbor index of the minimum x
  /// boundary is returned.
  ///
  VISKORES_EXEC viskores::IdComponent3 ClampNeighborIndex(
    const viskores::IdComponent3& neighbor) const
  {
    const viskores::Id3 fullIndex = this->IJK + neighbor;
    const viskores::Id3 clampedFullIndex = viskores::Max(
      viskores::Id3(0), viskores::Min(this->PointDimensions - viskores::Id3(1), fullIndex));
    return viskores::IdComponent3{ clampedFullIndex - this->IJK };
  }

  /// @copydoc ClampNeighborIndex
  VISKORES_EXEC viskores::IdComponent3 ClampNeighborIndex(viskores::IdComponent neighborI,
                                                          viskores::IdComponent neighborJ,
                                                          viskores::IdComponent neighborK) const
  {
    return this->ClampNeighborIndex(viskores::make_Vec(neighborI, neighborJ, neighborK));
  }

  /// Takes a local neighborhood index (in the ranges of -neighborhood size to neighborhood size)
  /// and returns the flat index of the equivalent point in the full data set. If the given value
  /// is out of range, the value is clamped to the nearest boundary. For example, if given a
  /// neighbor index that is past the minimum x range of the data, the index at the minimum x
  /// boundary is returned.
  ///
  VISKORES_EXEC viskores::Id NeighborIndexToFlatIndexClamp(
    const viskores::IdComponent3& neighbor) const
  {
    viskores::Id3 full = this->NeighborIndexToFullIndexClamp(neighbor);

    return (full[2] * this->PointDimensions[1] + full[1]) * this->PointDimensions[0] + full[0];
  }

  /// @copydoc NeighborIndexToFlatIndexClamp
  VISKORES_EXEC viskores::Id NeighborIndexToFlatIndexClamp(viskores::IdComponent neighborI,
                                                           viskores::IdComponent neighborJ,
                                                           viskores::IdComponent neighborK) const
  {
    return this->NeighborIndexToFlatIndexClamp(viskores::make_Vec(neighborI, neighborJ, neighborK));
  }

  /// Takes a local neighborhood index (in the ranges of -neighborhood size to neighborhood size)
  /// and returns the flat index of the equivalent point in the full data set. If the given value
  /// is out of range, the result is undefined.
  ///
  VISKORES_EXEC viskores::Id NeighborIndexToFlatIndex(const viskores::IdComponent3& neighbor) const
  {
    viskores::Id3 full = this->IJK + neighbor;
    return (full[2] * this->PointDimensions[1] + full[1]) * this->PointDimensions[0] + full[0];
  }

  /// @copydoc NeighborIndexToFlatIndex
  VISKORES_EXEC viskores::Id NeighborIndexToFlatIndex(viskores::IdComponent neighborI,
                                                      viskores::IdComponent neighborJ,
                                                      viskores::IdComponent neighborK) const
  {
    return this->NeighborIndexToFlatIndex(viskores::make_Vec(neighborI, neighborJ, neighborK));
  }

  /// The 3D index of the visited element.
  viskores::Id3 IJK;

  /// The dimensions of the elements in the mesh.
  viskores::Id3 PointDimensions;
};
}
} // namespace viskores::exec

#endif //viskores_exec_BoundaryState_h
