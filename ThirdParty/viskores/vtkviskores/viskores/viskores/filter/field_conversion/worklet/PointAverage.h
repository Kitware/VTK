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

#ifndef viskores_worklet_PointAverage_h
#define viskores_worklet_PointAverage_h

#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/VecTraits.h>

namespace viskores
{
namespace worklet
{

//simple functor that returns the average point value of a given
//cell based field.
class PointAverage : public viskores::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInCell inCellField,
                                FieldOutPoint outPointField);
  using ExecutionSignature = void(CellCount, _2, _3);
  using InputDomain = _1;

  template <typename CellValueVecType, typename OutType>
  VISKORES_EXEC void operator()(const viskores::IdComponent& numCells,
                                const CellValueVecType& cellValues,
                                OutType& average) const
  {
    using CellValueType = typename CellValueVecType::ComponentType;

    VISKORES_ASSERT(viskores::VecTraits<CellValueType>::GetNumberOfComponents(cellValues[0]) ==
                    viskores::VecTraits<OutType>::GetNumberOfComponents(average));

    average = cellValues[0];
    for (viskores::IdComponent cellIndex = 1; cellIndex < numCells; ++cellIndex)
    {
      average += cellValues[cellIndex];
    }

    using VTraits = viskores::VecTraits<OutType>;
    using OutComponentType = typename viskores::VecTraits<OutType>::ComponentType;
    const viskores::IdComponent numComponents = VTraits::GetNumberOfComponents(average);
    for (viskores::IdComponent compIndex = 0; compIndex < numComponents; ++compIndex)
    {
      VTraits::SetComponent(
        average,
        compIndex,
        static_cast<OutComponentType>(VTraits::GetComponent(average, compIndex) / numCells));
    }
  }
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_PointAverage_h
