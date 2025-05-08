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
#ifndef viskores_worklet_AveragePointNeighborhood_h
#define viskores_worklet_AveragePointNeighborhood_h

#include <viskores/VecTraits.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>

#include <viskores/worklet/WorkletPointNeighborhood.h>

namespace viskores
{
namespace worklet
{

class AveragePointNeighborhood : public viskores::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInNeighborhood inputField,
                                FieldOut outputField);
  using ExecutionSignature = _3(_2, Boundary);
  using InputDomain = _1;

  AveragePointNeighborhood(viskores::IdComponent radius)
  {
    VISKORES_ASSERT(radius > 0);
    this->BoundaryRadius = radius;
  }

  template <typename InputFieldPortalType>
  VISKORES_EXEC typename InputFieldPortalType::ValueType operator()(
    const viskores::exec::FieldNeighborhood<InputFieldPortalType>& inputField,
    const viskores::exec::BoundaryState& boundary) const
  {
    using T = typename InputFieldPortalType::ValueType;

    auto minIndices = boundary.MinNeighborIndices(this->BoundaryRadius);
    auto maxIndices = boundary.MaxNeighborIndices(this->BoundaryRadius);

    T sum(0);
    viskores::IdComponent size = 0;
    for (viskores::IdComponent i = minIndices[0]; i <= maxIndices[0]; i++)
    {
      for (viskores::IdComponent j = minIndices[1]; j <= maxIndices[1]; j++)
      {
        for (viskores::IdComponent k = minIndices[2]; k <= maxIndices[2]; k++)
        {
          sum = sum + inputField.Get(i, j, k);
          size++;
        }
      }
    }
    return static_cast<T>(sum / size);
  }

private:
  viskores::IdComponent BoundaryRadius;
};

} // viskores::worklet
} // viskores

#endif // viskores_worklet_AveragePointNeighborhood_h
