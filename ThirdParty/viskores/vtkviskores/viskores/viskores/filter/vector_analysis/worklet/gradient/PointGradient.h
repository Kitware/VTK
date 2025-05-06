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

#ifndef viskores_worklet_gradient_PointGradient_h
#define viskores_worklet_gradient_PointGradient_h

#include <viskores/exec/CellDerivative.h>
#include <viskores/exec/ParametricCoordinates.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <utility>
#include <viskores/filter/vector_analysis/worklet/gradient/GradientOutput.h>


namespace viskores
{
namespace worklet
{
namespace gradient
{

struct PointGradient : public viskores::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn,
                                WholeCellSetIn<Cell, Point>,
                                WholeArrayIn pointCoordinates,
                                WholeArrayIn inputField,
                                GradientOutputs outputFields);

  using ExecutionSignature = void(CellCount, CellIndices, WorkIndex, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename FromIndexType,
            typename CellSetInType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename GradientOutType>
  VISKORES_EXEC void operator()(const viskores::IdComponent& numCells,
                                const FromIndexType& cellIds,
                                const viskores::Id& pointId,
                                const CellSetInType& geometry,
                                const WholeCoordinatesIn& pointCoordinates,
                                const WholeFieldIn& inputField,
                                GradientOutType& outputGradient) const
  {
    // Use optimized ThreadIndicesTopologyMap
    using CellThreadIndices =
      viskores::exec::arg::ThreadIndicesTopologyMap<CellSetInType,
                                                    viskores::exec::arg::DefaultScatterAndMaskTag>;

    using ValueType = typename WholeFieldIn::ValueType;
    using CellShapeTag = typename CellSetInType::CellShapeTag;

    viskores::Vec<ValueType, 3> gradient(ValueType(0.0));
    for (viskores::IdComponent i = 0; i < numCells; ++i)
    {
      const viskores::Id cellId = cellIds[i];
      CellThreadIndices cellIndices(cellId, cellId, 0, cellId, geometry);

      const CellShapeTag cellShape = cellIndices.GetCellShape();

      // compute the parametric coordinates for the current point
      const auto wCoords = this->GetValues(cellIndices, pointCoordinates);
      const auto field = this->GetValues(cellIndices, inputField);

      const viskores::IdComponent pointIndexForCell =
        this->GetPointIndexForCell(cellIndices, pointId);

      this->ComputeGradient(cellShape, pointIndexForCell, wCoords, field, gradient);
    }

    if (numCells != 0)
    {
      using BaseGradientType = typename viskores::VecTraits<ValueType>::BaseComponentType;
      const BaseGradientType invNumCells =
        static_cast<BaseGradientType>(1.) / static_cast<BaseGradientType>(numCells);

      gradient[0] = gradient[0] * invNumCells;
      gradient[1] = gradient[1] * invNumCells;
      gradient[2] = gradient[2] * invNumCells;
    }
    outputGradient = gradient;
  }

private:
  template <typename CellShapeTag,
            typename PointCoordVecType,
            typename FieldInVecType,
            typename OutValueType>
  inline VISKORES_EXEC void ComputeGradient(CellShapeTag cellShape,
                                            const viskores::IdComponent& pointIndexForCell,
                                            const PointCoordVecType& wCoords,
                                            const FieldInVecType& field,
                                            viskores::Vec<OutValueType, 3>& gradient) const
  {
    viskores::Vec3f pCoords;
    viskores::exec::ParametricCoordinatesPoint(
      wCoords.GetNumberOfComponents(), pointIndexForCell, cellShape, pCoords);

    //we need to add this to a return value
    viskores::Vec<OutValueType, 3> pointGradient;
    auto status = viskores::exec::CellDerivative(field, wCoords, pCoords, cellShape, pointGradient);
    if (status == viskores::ErrorCode::Success)
    {
      gradient += pointGradient;
    }
  }

  template <typename ThreadIndicesType>
  VISKORES_EXEC viskores::IdComponent GetPointIndexForCell(const ThreadIndicesType& indices,
                                                           viskores::Id pointId) const
  {
    viskores::IdComponent result = 0;
    const auto& topo = indices.GetIndicesIncident();
    for (viskores::IdComponent i = 0; i < topo.GetNumberOfComponents(); ++i)
    {
      if (topo[i] == pointId)
      {
        result = i;
      }
    }
    return result;
  }

  template <typename ThreadIndicesType, typename WholeFieldIn>
  VISKORES_EXEC auto GetValues(const ThreadIndicesType& indices, const WholeFieldIn& in) const
  {
    //the current problem is that when the topology is structured
    //we are passing in an viskores::Id when it wants a Id2 or an Id3 that
    //represents the flat index of the topology
    using Fetch = viskores::exec::arg::Fetch<viskores::exec::arg::FetchTagArrayTopologyMapIn,
                                             viskores::exec::arg::AspectTagDefault,
                                             WholeFieldIn>;
    Fetch fetch;
    return fetch.Load(indices, in);
  }
};
}
}
}

#endif
