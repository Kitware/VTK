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
#ifndef viskores_worklet_PointMerge_h
#define viskores_worklet_PointMerge_h

#include <viskores/filter/clean_grid/worklet/RemoveUnusedPoints.h>
#include <viskores/worklet/AverageByKey.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherReduceByKey.h>
#include <viskores/worklet/Keys.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletReduceByKey.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/ExecutionAndControlObjectBase.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/Bounds.h>
#include <viskores/Hash.h>
#include <viskores/Math.h>
#include <viskores/VectorAnalysis.h>

namespace viskores
{
namespace worklet
{

class PointMerge
{
public:
  // This class can take point worldCoords as inputs and return the bin index of the enclosing bin.
  class BinLocator : public viskores::cont::ExecutionAndControlObjectBase
  {
    viskores::Vec3f_64 Offset;
    viskores::Vec3f_64 Scale;

#ifdef VISKORES_USE_64BIT_IDS
    // IEEE double precision floating point as 53 bits for the significand, so it would not be
    // possible to represent a number with more precision than that. We also back off a few bits to
    // avoid potential issues with numerical imprecision in the scaling.
    static constexpr viskores::IdComponent BitsPerDimension = 50;
#else
    static constexpr viskores::IdComponent BitsPerDimension = 31;
#endif
    static constexpr viskores::Id MaxBinsPerDimension =
      static_cast<viskores::Id>((1LL << BitsPerDimension) - 1);

  public:
    VISKORES_CONT BinLocator()
      : Offset(0.0)
      , Scale(0.0)
    {
    }

    VISKORES_CONT
    static viskores::Vec3f_64 ComputeBinWidths(const viskores::Bounds& bounds,
                                               viskores::Float64 delta)
    {
      const viskores::Vec3f_64 boundLengths(
        bounds.X.Length() + delta, bounds.Y.Length() + delta, bounds.Z.Length() + delta);
      viskores::Vec3f_64 binWidths;
      for (viskores::IdComponent dimIndex = 0; dimIndex < 3; ++dimIndex)
      {
        if (boundLengths[dimIndex] > viskores::Epsilon64())
        {
          viskores::Float64 minBinWidth = boundLengths[dimIndex] / (MaxBinsPerDimension - 1);
          if (minBinWidth < (2 * delta))
          {
            // We can accurately represent delta with the precision of the bin indices. The bin
            // size is 2*delta, which means we scale the (offset) point coordinates by 1/delta to
            // get the bin index.
            binWidths[dimIndex] = 2.0 * delta;
          }
          else
          {
            // Scale the (offset) point coordinates by 1/minBinWidth, which will give us bin
            // indices between 0 and MaxBinsPerDimension - 1.
            binWidths[dimIndex] = minBinWidth;
          }
        }
        else
        {
          // Bounds are essentially 0 in this dimension. The scale does not matter so much.
          binWidths[dimIndex] = 1.0;
        }
      }
      return binWidths;
    }

    // Constructs a BinLocator such that all bins are at least 2*delta large. The bins might be
    // made larger than that if there would be too many bins for the precision of viskores::Id.
    VISKORES_CONT
    BinLocator(const viskores::Bounds& bounds, viskores::Float64 delta = 0.0)
      : Offset(bounds.X.Min, bounds.Y.Min, bounds.Z.Min)
    {
      const viskores::Vec3f_64 binWidths = ComputeBinWidths(bounds, delta);
      this->Scale = viskores::Vec3f_64(1.0) / binWidths;
    }

    // Shifts the grid by delta in the specified directions. This will allow the bins to cover
    // neighbors that straddled the boundaries of the original.
    VISKORES_CONT
    BinLocator ShiftBins(const viskores::Bounds& bounds,
                         viskores::Float64 delta,
                         const viskores::Vec<bool, 3>& directions)
    {
      const viskores::Vec3f_64 binWidths = ComputeBinWidths(bounds, delta);
      BinLocator shiftedLocator(*this);
      for (viskores::IdComponent dimIndex = 0; dimIndex < 3; ++dimIndex)
      {
        if (directions[dimIndex])
        {
          shiftedLocator.Offset[dimIndex] -= (0.5 * binWidths[dimIndex]);
        }
      }
      return shiftedLocator;
    }

    template <typename T>
    VISKORES_EXEC_CONT viskores::Id3 FindBin(const viskores::Vec<T, 3>& worldCoords) const
    {
      viskores::Vec3f_64 relativeCoords = (worldCoords - this->Offset) * this->Scale;

      return viskores::Id3(viskores::Floor(relativeCoords));
    }

    // Because this class is a POD, we can reuse it in both control and execution environments.

    BinLocator PrepareForExecution(viskores::cont::DeviceAdapterId, viskores::cont::Token&) const
    {
      return *this;
    }

    BinLocator PrepareForControl() const { return *this; }
  };

  // Converts point coordinates to a hash that represents the bin.
  struct CoordsToHash : public viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn pointCoordinates,
                                  ExecObject binLocator,
                                  FieldOut hashesOut);
    using ExecutionSignature = void(_1, _2, _3);

    template <typename T>
    VISKORES_EXEC void operator()(const viskores::Vec<T, 3>& coordiantes,
                                  const BinLocator binLocator,
                                  viskores::HashType& hashOut) const
    {
      viskores::Id3 binId = binLocator.FindBin(coordiantes);
      hashOut = viskores::Hash(binId);
    }
  };

  class FindNeighbors : public viskores::worklet::WorkletReduceByKey
  {
    viskores::Float64 DeltaSquared;
    bool FastCheck;

  public:
    VISKORES_CONT
    FindNeighbors(bool fastCheck = true, viskores::Float64 delta = viskores::Epsilon64())
      : DeltaSquared(delta * delta)
      , FastCheck(fastCheck)
    {
    }

    using ControlSignature = void(KeysIn keys,
                                  ValuesInOut pointIndices,
                                  ValuesInOut pointCoordinates,
                                  ExecObject binLocator,
                                  ValuesOut neighborIndices);
    using ExecutionSignature = void(_2, _3, _4, _5);

    template <typename IndexVecInType, typename CoordinateVecInType, typename IndexVecOutType>
    VISKORES_EXEC void operator()(IndexVecInType& pointIndices,
                                  CoordinateVecInType& pointCoordinates,
                                  const BinLocator& binLocator,
                                  IndexVecOutType& neighborIndices) const
    {
      // For each point we are going to find all points close enough to be considered neighbors. We
      // record the neighbors by filling in the same index into neighborIndices. That is, if two
      // items in neighborIndices have the same value, they should be considered neighbors.
      // Otherwise, they should not. We will use the "local" index, which refers to index in the
      // vec-like objects passed into this worklet. This allows us to quickly identify the local
      // point without sorting through the global indices.

      using CoordType = typename CoordinateVecInType::ComponentType;

      viskores::IdComponent numPoints = pointIndices.GetNumberOfComponents();
      VISKORES_ASSERT(numPoints == pointCoordinates.GetNumberOfComponents());
      VISKORES_ASSERT(numPoints == neighborIndices.GetNumberOfComponents());

      // Initially, set every point to be its own neighbor.
      for (viskores::IdComponent i = 0; i < numPoints; ++i)
      {
        neighborIndices[i] = i;
      }

      // Iterate over every point and look for neighbors. Only need to look to numPoints-1 since we
      // only need to check points after the current index (earlier points are already checked).
      for (viskores::IdComponent i = 0; i < (numPoints - 1); ++i)
      {
        CoordType p0 = pointCoordinates[i];
        viskores::Id3 bin0 = binLocator.FindBin(p0);

        // Check all points after this one. (All those before already checked themselves to this.)
        for (viskores::IdComponent j = i + 1; j < numPoints; ++j)
        {
          if (neighborIndices[i] == neighborIndices[j])
          {
            // We have already identified these points as neighbors. Can skip the check.
            continue;
          }
          CoordType p1 = pointCoordinates[j];
          viskores::Id3 bin1 = binLocator.FindBin(p1);

          // Check to see if these points should be considered neighbors. First, check to make sure
          // that they are in the same bin. If they are not, then they cannot be neighbors. Next,
          // check the FastCheck flag. If fast checking is on, then all points in the same bin are
          // considered neighbors. Otherwise, check that the distance is within the specified
          // delta. If so, mark them as neighbors.
          if ((bin0 == bin1) &&
              (this->FastCheck || (this->DeltaSquared >= viskores::MagnitudeSquared(p0 - p1))))
          {
            // The two points should be merged. But we also might need to merge larger
            // neighborhoods.
            if (neighborIndices[j] == j)
            {
              // Second point not yet merged into another neighborhood. We can just take it.
              neighborIndices[j] = neighborIndices[i];
            }
            else
            {
              // The second point is already part of a neighborhood. Merge the neighborhood with
              // the largest index into the neighborhood with the smaller index.
              viskores::IdComponent neighborhoodToGrow;
              viskores::IdComponent neighborhoodToAbsorb;
              if (neighborIndices[i] < neighborIndices[j])
              {
                neighborhoodToGrow = neighborIndices[i];
                neighborhoodToAbsorb = neighborIndices[j];
              }
              else
              {
                neighborhoodToGrow = neighborIndices[j];
                neighborhoodToAbsorb = neighborIndices[i];
              }

              // Change all neighborhoodToAbsorb indices to neighborhoodToGrow.
              for (viskores::IdComponent k = neighborhoodToAbsorb; k < numPoints; ++k)
              {
                if (neighborIndices[k] == neighborhoodToAbsorb)
                {
                  neighborIndices[k] = neighborhoodToGrow;
                }
              }
            }
          } // if merge points
        }   // for each p1
      }     // for each p0

      // We have finished grouping neighbors. neighborIndices contains a unique local index for
      // each neighbor group. Now find the average (centroid) point coordinates for each group and
      // write those coordinates back into the coordinates array. Also modify the point indices
      // so that all indices of a group are the same. (This forms a map from old point indices to
      // merged point indices.)
      for (viskores::IdComponent i = 0; i < numPoints; ++i)
      {
        viskores::IdComponent neighborhood = neighborIndices[i];
        if (i == neighborhood)
        {
          // Found a new group. Find the centroid.
          CoordType centroid = pointCoordinates[i];
          viskores::IdComponent numInGroup = 1;
          for (viskores::IdComponent j = i + 1; j < numPoints; ++j)
          {
            if (neighborhood == neighborIndices[j])
            {
              centroid = centroid + pointCoordinates[j];
              ++numInGroup;
            }
          }
          centroid = centroid / numInGroup;

          // Now that we have the centroid, write new point coordinates and index.
          viskores::Id groupIndex = pointIndices[i];
          pointCoordinates[i] = centroid;
          for (viskores::IdComponent j = i + 1; j < numPoints; ++j)
          {
            if (neighborhood == neighborIndices[j])
            {
              pointCoordinates[j] = centroid;
              pointIndices[j] = groupIndex;
            }
          }
        }
      }
    }
  };

  struct BuildPointInputToOutputMap : viskores::worklet::WorkletReduceByKey
  {
    using ControlSignature = void(KeysIn, ValuesOut PointInputToOutputMap);
    using ExecutionSignature = void(InputIndex, _2);

    template <typename MapPortalType>
    VISKORES_EXEC void operator()(viskores::Id newIndex, MapPortalType outputIndices) const
    {
      const viskores::IdComponent numIndices = outputIndices.GetNumberOfComponents();
      for (viskores::IdComponent i = 0; i < numIndices; ++i)
      {
        outputIndices[i] = newIndex;
      }
    }
  };

private:
  template <typename T>
  VISKORES_CONT static void RunOneIteration(
    viskores::Float64 delta,      // Distance to consider two points coincident
    bool fastCheck,               // If true, approximate distances are used
    const BinLocator& binLocator, // Used to find nearby points
    viskores::cont::ArrayHandle<viskores::Vec<T, 3>>&
      points, // coordinates, modified to merge close
    const viskores::cont::ArrayHandle<viskores::Id>&
      indexNeighborMap) // identifies each neighbor group, updated
  {
    viskores::cont::Invoker invoker;

    viskores::cont::ArrayHandle<viskores::HashType> hashes;
    invoker(CoordsToHash(), points, binLocator, hashes);

    viskores::worklet::Keys<HashType> keys(hashes);

    // Really just scratch space
    viskores::cont::ArrayHandle<viskores::IdComponent> neighborIndices;

    invoker(
      FindNeighbors(fastCheck, delta), keys, indexNeighborMap, points, binLocator, neighborIndices);
  }

public:
  template <typename T>
  VISKORES_CONT void Run(viskores::Float64 delta, // Distance to consider two points coincident
                         bool fastCheck,          // If true, approximate distances are used
                         const viskores::Bounds& bounds, // Bounds of points
                         viskores::cont::ArrayHandle<viskores::Vec<T, 3>>&
                           points) // coordinates, modified to merge close
  {
    viskores::cont::Invoker invoker;

    BinLocator binLocator(bounds, delta);

    viskores::cont::ArrayHandle<viskores::Id> indexNeighborMap;
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(points.GetNumberOfValues()),
                              indexNeighborMap);

    this->RunOneIteration(delta, fastCheck, binLocator, points, indexNeighborMap);

    if (!fastCheck)
    {
      // Run the algorithm again after shifting the bins to capture nearby points that straddled
      // the previous bins.
      this->RunOneIteration(
        delta,
        fastCheck,
        binLocator.ShiftBins(bounds, delta, viskores::make_Vec(true, false, false)),
        points,
        indexNeighborMap);
      this->RunOneIteration(
        delta,
        fastCheck,
        binLocator.ShiftBins(bounds, delta, viskores::make_Vec(false, true, false)),
        points,
        indexNeighborMap);
      this->RunOneIteration(
        delta,
        fastCheck,
        binLocator.ShiftBins(bounds, delta, viskores::make_Vec(false, false, true)),
        points,
        indexNeighborMap);
      this->RunOneIteration(
        delta,
        fastCheck,
        binLocator.ShiftBins(bounds, delta, viskores::make_Vec(true, true, false)),
        points,
        indexNeighborMap);
      this->RunOneIteration(
        delta,
        fastCheck,
        binLocator.ShiftBins(bounds, delta, viskores::make_Vec(true, false, true)),
        points,
        indexNeighborMap);
      this->RunOneIteration(
        delta,
        fastCheck,
        binLocator.ShiftBins(bounds, delta, viskores::make_Vec(false, true, true)),
        points,
        indexNeighborMap);
      this->RunOneIteration(
        delta,
        fastCheck,
        binLocator.ShiftBins(bounds, delta, viskores::make_Vec(true, true, true)),
        points,
        indexNeighborMap);
    }

    this->MergeKeys = viskores::worklet::Keys<viskores::Id>(indexNeighborMap);

    invoker(BuildPointInputToOutputMap(), this->MergeKeys, this->PointInputToOutputMap);

    // Need to pull out the unique point coordiantes
    viskores::cont::ArrayHandle<viskores::Vec<T, 3>> uniquePointCoordinates;
    viskores::cont::ArrayCopy(
      viskores::cont::make_ArrayHandlePermutation(this->MergeKeys.GetUniqueKeys(), points),
      uniquePointCoordinates);
    points = uniquePointCoordinates;
  }

  template <typename TL, typename SL>
  VISKORES_CONT void Run(
    viskores::Float64 delta,        // Distance to consider two points coincident
    bool fastCheck,                 // If true, approximate distances are used
    const viskores::Bounds& bounds, // Bounds of points
    viskores::cont::UncertainArrayHandle<TL, SL>& points) // coordinates, modified to merge close
  {
    // Get a cast to a concrete set of point coordiantes so that it can be modified in place
    viskores::cont::ArrayHandle<viskores::Vec3f> concretePoints;
    viskores::cont::ArrayCopyShallowIfPossible(points, concretePoints);

    Run(delta, fastCheck, bounds, concretePoints);

    // Make sure that the modified points are reflected back in the variant array.
    points = concretePoints;
  }

  VISKORES_CONT void Run(
    viskores::Float64 delta,                    // Distance to consider two points coincident
    bool fastCheck,                             // If true, approximate distances are used
    const viskores::Bounds& bounds,             // Bounds of points
    viskores::cont::UnknownArrayHandle& points) // coordinates, modified to merge close
  {
    // Get a cast to a concrete set of point coordiantes so that it can be modified in place
    viskores::cont::ArrayHandle<viskores::Vec3f> concretePoints;
    viskores::cont::ArrayCopyShallowIfPossible(points, concretePoints);

    Run(delta, fastCheck, bounds, concretePoints);

    // Make sure that the modified points are reflected back in the variant array.
    points = concretePoints;
  }

  template <typename ShapeStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VISKORES_CONT viskores::cont::
    CellSetExplicit<ShapeStorage, VISKORES_DEFAULT_CONNECTIVITY_STORAGE_TAG, OffsetsStorage>
    MapCellSet(
      const viskores::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>&
        inCellSet) const
  {
    return viskores::worklet::RemoveUnusedPoints::MapCellSet(
      inCellSet, this->PointInputToOutputMap, this->MergeKeys.GetInputRange());
  }

  viskores::worklet::Keys<viskores::Id> GetMergeKeys() const { return this->MergeKeys; }

private:
  viskores::worklet::Keys<viskores::Id> MergeKeys;
  viskores::cont::ArrayHandle<viskores::Id> PointInputToOutputMap;
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_PointMerge_h
