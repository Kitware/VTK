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

#ifndef viskores_worklet_gradient_CellGradient_h
#define viskores_worklet_gradient_CellGradient_h

#include <viskores/exec/CellDerivative.h>
#include <viskores/exec/ParametricCoordinates.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/filter/vector_analysis/worklet/gradient/GradientOutput.h>

namespace viskores
{
namespace worklet
{
namespace gradient
{

struct CellGradient : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn,
                                FieldInPoint pointCoordinates,
                                FieldInPoint inputField,
                                GradientOutputs outputFields);

  using ExecutionSignature = void(CellShape, PointCount, _2, _3, _4);
  using InputDomain = _1;

  template <typename CellTagType,
            typename PointCoordVecType,
            typename FieldInVecType,
            typename GradientOutType>
  VISKORES_EXEC void operator()(CellTagType shape,
                                viskores::IdComponent pointCount,
                                const PointCoordVecType& wCoords,
                                const FieldInVecType& field,
                                GradientOutType& outputGradient) const
  {
    viskores::Vec3f center;
    viskores::exec::ParametricCoordinatesCenter(pointCount, shape, center);

    viskores::exec::CellDerivative(field, wCoords, center, shape, outputGradient);
  }
};
}
}
}

#endif
