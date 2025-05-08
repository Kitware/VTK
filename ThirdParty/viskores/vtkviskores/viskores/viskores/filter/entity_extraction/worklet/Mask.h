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
#ifndef viskores_m_worklet_Mask_h
#define viskores_m_worklet_Mask_h

#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/CellSetPermutation.h>

namespace viskores
{
namespace worklet
{

// Subselect points using stride for now, creating new cellset of vertices
class Mask
{
public:
  template <typename CellSetType>
  viskores::cont::CellSetPermutation<CellSetType> Run(const CellSetType& cellSet,
                                                      const viskores::Id stride)
  {
    using OutputType = viskores::cont::CellSetPermutation<CellSetType>;

    viskores::Id numberOfInputCells = cellSet.GetNumberOfCells();
    viskores::Id numberOfSampledCells = numberOfInputCells / stride;
    viskores::cont::ArrayHandleCounting<viskores::Id> strideArray(0, stride, numberOfSampledCells);

    viskores::cont::ArrayCopy(strideArray, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet);
  }

  viskores::cont::ArrayHandle<viskores::Id> GetValidCellIds() const { return this->ValidCellIds; }

private:
  viskores::cont::ArrayHandle<viskores::Id> ValidCellIds;
};
}
} // namespace viskores::worklet

#endif // viskores_m_worklet_Mask_h
