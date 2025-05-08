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

#ifndef viskores_worklet_CellAverage_h
#define viskores_worklet_CellAverage_h

#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/VecTraits.h>

namespace viskores
{
namespace worklet
{

//simple functor that returns the average point value as a cell field
class CellAverage : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldInPoint inPoints, FieldOutCell outCells);
  using ExecutionSignature = void(PointCount, _2, _3);
  using InputDomain = _1;

  template <typename PointValueVecType, typename OutType>
  VISKORES_EXEC void operator()(const viskores::IdComponent& numPoints,
                                const PointValueVecType& pointValues,
                                OutType& average) const
  {
    using PointValueType = typename PointValueVecType::ComponentType;

    VISKORES_ASSERT(viskores::VecTraits<PointValueType>::GetNumberOfComponents(pointValues[0]) ==
                    viskores::VecTraits<OutType>::GetNumberOfComponents(average));

    average = pointValues[0];
    for (viskores::IdComponent pointIndex = 1; pointIndex < numPoints; ++pointIndex)
    {
      average += pointValues[pointIndex];
    }

    using VTraits = viskores::VecTraits<OutType>;
    using OutComponentType = typename VTraits::ComponentType;
    const viskores::IdComponent numComponents = VTraits::GetNumberOfComponents(average);
    for (viskores::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      VTraits::SetComponent(
        average,
        cIndex,
        static_cast<OutComponentType>(VTraits::GetComponent(average, cIndex) / numPoints));
    }
  }
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_CellAverage_h
