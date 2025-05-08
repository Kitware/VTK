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
#ifndef viskores_m_worklet_MaskPoints_h
#define viskores_m_worklet_MaskPoints_h

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>

namespace viskores
{
namespace worklet
{

// Subselect points using stride for now, creating new cellset of vertices
class MaskPoints
{
public:
  template <typename CellSetType>
  viskores::cont::CellSetSingleType<> Run(const CellSetType& cellSet, const viskores::Id stride)
  {
    viskores::Id numberOfInputPoints = cellSet.GetNumberOfPoints();
    viskores::Id numberOfSampledPoints = numberOfInputPoints / stride;
    viskores::cont::ArrayHandleCounting<viskores::Id> strideArray(0, stride, numberOfSampledPoints);

    viskores::cont::ArrayHandle<viskores::Id> pointIds;
    viskores::cont::ArrayCopy(strideArray, pointIds);

    // Make CellSetSingleType with VERTEX at each point id
    viskores::cont::CellSetSingleType<> outCellSet;
    outCellSet.Fill(numberOfInputPoints, viskores::CellShapeTagVertex::Id, 1, pointIds);

    return outCellSet;
  }
};
}
} // namespace viskores::worklet

#endif // viskores_m_worklet_MaskPoints_h
