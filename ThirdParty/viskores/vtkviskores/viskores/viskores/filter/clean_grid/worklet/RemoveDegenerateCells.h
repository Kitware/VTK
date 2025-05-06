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
#ifndef viskores_worklet_RemoveDegeneratePolygons_h
#define viskores_worklet_RemoveDegeneratePolygons_h

#include <viskores/worklet/DispatcherMapTopology.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetPermutation.h>
#include <viskores/cont/UncertainCellSet.h>

#include <viskores/worklet/CellDeepCopy.h>

#include <viskores/CellTraits.h>

#include <viskores/exec/CellFace.h>

namespace viskores
{
namespace worklet
{

struct RemoveDegenerateCells
{
  struct IdentifyDegenerates : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn, FieldOutCell);
    using ExecutionSignature = _2(CellShape, PointIndices);
    using InputDomain = _1;

    template <viskores::IdComponent dimensionality, typename CellShapeTag, typename PointVecType>
    VISKORES_EXEC bool CheckForDimensionality(
      viskores::CellTopologicalDimensionsTag<dimensionality>,
      CellShapeTag,
      PointVecType&& pointIds) const
    {
      const viskores::IdComponent numPoints = pointIds.GetNumberOfComponents();
      viskores::IdComponent numUnduplicatedPoints = 0;
      // Skip first point if it is the same as the last.
      for (viskores::IdComponent localPointId = ((pointIds[0] != pointIds[numPoints - 1]) ? 0 : 1);
           localPointId < numPoints;
           ++localPointId)
      {
        ++numUnduplicatedPoints;
        if (numUnduplicatedPoints >= dimensionality + 1)
        {
          return true;
        }
        // Skip over any repeated points. Assume any repeated points are adjacent.
        while ((localPointId < numPoints - 1) &&
               (pointIds[localPointId] == pointIds[localPointId + 1]))
        {
          ++localPointId;
        }
      }
      return false;
    }

    template <typename CellShapeTag, typename PointVecType>
    VISKORES_EXEC bool CheckForDimensionality(viskores::CellTopologicalDimensionsTag<0>,
                                              CellShapeTag,
                                              PointVecType&&) const
    {
      return true;
    }

    template <typename CellShapeTag, typename PointVecType>
    VISKORES_EXEC bool CheckForDimensionality(viskores::CellTopologicalDimensionsTag<3>,
                                              CellShapeTag shape,
                                              PointVecType&& pointIds) const
    {
      viskores::IdComponent numFaces;
      viskores::exec::CellFaceNumberOfFaces(shape, numFaces);
      viskores::Id numValidFaces = 0;
      for (viskores::IdComponent faceId = 0; faceId < numFaces; ++faceId)
      {
        if (this->CheckForDimensionality(viskores::CellTopologicalDimensionsTag<2>(),
                                         viskores::CellShapeTagPolygon(),
                                         pointIds))
        {
          ++numValidFaces;
          if (numValidFaces > 2)
          {
            return true;
          }
        }
      }
      return false;
    }

    template <typename CellShapeTag, typename PointIdVec>
    VISKORES_EXEC bool operator()(CellShapeTag shape, const PointIdVec& pointIds) const
    {
      using Traits = viskores::CellTraits<CellShapeTag>;
      return this->CheckForDimensionality(
        typename Traits::TopologicalDimensionsTag(), shape, pointIds);
    }

    template <typename PointIdVec>
    VISKORES_EXEC bool operator()(viskores::CellShapeTagGeneric shape, PointIdVec&& pointIds) const
    {
      bool passCell = true;
      switch (shape.Id)
      {
        viskoresGenericCellShapeMacro(passCell = (*this)(CellShapeTag(), pointIds));
        default:
          // Raise an error for unknown cell type? Pass if we don't know.
          passCell = true;
      }
      return passCell;
    }
  };

  template <typename CellSetType>
  viskores::cont::CellSetExplicit<> Run(const CellSetType& cellSet)
  {
    viskores::cont::ArrayHandle<bool> passFlags;
    DispatcherMapTopology<IdentifyDegenerates> dispatcher;
    dispatcher.Invoke(cellSet, passFlags);

    viskores::cont::ArrayHandleCounting<viskores::Id> indices =
      viskores::cont::make_ArrayHandleCounting(
        viskores::Id(0), viskores::Id(1), passFlags.GetNumberOfValues());
    viskores::cont::Algorithm::CopyIf(
      viskores::cont::ArrayHandleIndex(passFlags.GetNumberOfValues()),
      passFlags,
      this->ValidCellIds);

    viskores::cont::CellSetPermutation<CellSetType> permutation(this->ValidCellIds, cellSet);
    viskores::cont::CellSetExplicit<> output;
    viskores::worklet::CellDeepCopy::Run(permutation, output);
    return output;
  }

  template <typename CellSetList>
  viskores::cont::CellSetExplicit<> Run(
    const viskores::cont::UncertainCellSet<CellSetList>& cellSet)
  {
    viskores::cont::CellSetExplicit<> output;
    cellSet.CastAndCall([&](const auto& concrete) { output = this->Run(concrete); });

    return output;
  }

  viskores::cont::ArrayHandle<viskores::Id> GetValidCellIds() const { return this->ValidCellIds; }

private:
  viskores::cont::ArrayHandle<viskores::Id> ValidCellIds;
};
}
}

#endif //viskores_worklet_RemoveDegeneratePolygons_h
