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
#ifndef viskores_worklet_Shrink_h
#define viskores_worklet_Shrink_h

#include <viskores/worklet/CellDeepCopy.h>
#include <viskores/worklet/ScatterCounting.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/Invoker.h>
#include <viskores/exec/ParametricCoordinates.h>


namespace viskores
{
namespace worklet
{
class Shrink
{
public:
  struct PrepareCellsForShrink : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn,
                                  FieldOutCell numPoints,
                                  FieldOutCell centroids,
                                  FieldOutCell shapes,
                                  FieldInPoint coords);
    using ExecutionSignature =
      void(PointCount, _2 numPoints, _3 centroids, _4 shapes, _5 coords, CellShape);

    using InputDomain = _1;

    template <typename CoordsArrayType, typename ShapeIdType, typename ShapeTagType>
    VISKORES_EXEC void operator()(viskores::IdComponent numPointsInCell,
                                  viskores::IdComponent& numPoints,
                                  viskores::Vec3f& centroids,
                                  ShapeIdType& shapes,
                                  const CoordsArrayType& coords,
                                  ShapeTagType cellShape) const
    {
      numPoints = numPointsInCell;
      shapes = cellShape.Id;

      viskores::Vec3f cellCenter;
      viskores::exec::ParametricCoordinatesCenter(numPoints, cellShape, cellCenter);
      viskores::exec::CellInterpolate(coords, cellCenter, cellShape, centroids);
    }
  };

  struct ComputeNewPoints : viskores::worklet::WorkletVisitCellsWithPoints
  {
    ComputeNewPoints(viskores::FloatDefault shrinkFactor)
      : ShrinkFactor(shrinkFactor)
    {
    }
    using ControlSignature = void(CellSetIn,
                                  FieldInCell offsets,
                                  FieldInCell centroids,
                                  FieldOutCell oldPointsMapping,
                                  FieldOutCell newPoints,
                                  FieldOutCell newCoords,
                                  FieldInPoint coords);
    using ExecutionSignature = void(_2 offsets,
                                    _3 centroids,
                                    _4 oldPointsMapping,
                                    _5 newPoints,
                                    _6 newCoords,
                                    _7 coords,
                                    VisitIndex localPointNum,
                                    PointIndices globalPointIndex);
    using InputDomain = _1;

    using ScatterType = viskores::worklet::ScatterCounting;

    template <typename PointIndicesVecType, typename CoordsArrayTypeIn, typename CoordsArrayTypeOut>
    VISKORES_EXEC void operator()(const viskores::Id& offsets,
                                  const viskores::Vec3f& centroids,
                                  viskores::Id& oldPointsMapping,
                                  viskores::Id& newPoints,
                                  CoordsArrayTypeOut& newCoords,
                                  const CoordsArrayTypeIn& coords,
                                  viskores::IdComponent localPtIndex,
                                  const PointIndicesVecType& globalPointIndex) const
    {
      newPoints = offsets + localPtIndex;
      oldPointsMapping = globalPointIndex[localPtIndex];
      newCoords = centroids + this->ShrinkFactor * (coords[localPtIndex] - centroids);
    }

  private:
    viskores::FloatDefault ShrinkFactor;
  };

  template <typename CellSetType,
            typename CoordsComType,
            typename CoordsInStorageType,
            typename CoordsOutStorageType,
            typename OldPointsMappingType,
            typename NewCellSetType>
  void Run(
    const CellSetType& oldCellset,
    const viskores::FloatDefault shinkFactor,
    const viskores::cont::ArrayHandle<viskores::Vec<CoordsComType, 3>, CoordsInStorageType>&
      oldCoords,
    viskores::cont::ArrayHandle<viskores::Vec<CoordsComType, 3>, CoordsOutStorageType>& newCoords,
    viskores::cont::ArrayHandle<viskores::Id, OldPointsMappingType>& oldPointsMapping,
    NewCellSetType& newCellset)
  {
    viskores::cont::Invoker invoke;

    // First pass : count the new number of points per cell, shapes and compute centroids
    viskores::cont::ArrayHandle<viskores::IdComponent> cellPointCount;
    viskores::cont::ArrayHandle<viskores::Vec3f> centroids;
    viskores::cont::CellSetExplicit<>::ShapesArrayType shapeArray;
    invoke(PrepareCellsForShrink{}, oldCellset, cellPointCount, centroids, shapeArray, oldCoords);


    // Second pass : compute new point positions and mappings to input points
    viskores::cont::ArrayHandle<viskores::Id> newPoints;
    viskores::worklet::ScatterCounting scatter(cellPointCount, true);
    viskores::cont::ArrayHandle<viskores::Id> offsets = scatter.GetInputToOutputMap();
    viskores::Id totalPoints = scatter.GetOutputRange(cellPointCount.GetNumberOfValues());

    ComputeNewPoints worklet = ComputeNewPoints(shinkFactor);
    invoke(worklet,
           scatter,
           oldCellset,
           offsets,
           centroids,
           oldPointsMapping,
           newPoints,
           newCoords,
           oldCoords);

    newCellset.Fill(totalPoints,
                    shapeArray,
                    newPoints,
                    viskores::cont::ConvertNumComponentsToOffsets(cellPointCount));
  }
};
} // namespace viskores::worklet
} // namespace viskores

#endif // viskores_worklet_Shrink_h
