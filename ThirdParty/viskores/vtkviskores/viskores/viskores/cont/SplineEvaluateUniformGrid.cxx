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

#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/SplineEvaluateUniformGrid.h>

namespace viskores
{
namespace cont
{

viskores::exec::SplineEvaluateUniformGrid SplineEvaluateUniformGrid::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token) const
{
  using UniformCoordsType = viskores::cont::ArrayHandleUniformPointCoordinates;

  if (!this->DataSet.GetCoordinateSystem(0).GetData().IsType<UniformCoordsType>())
    throw viskores::cont::ErrorBadType("Coordinates are not uniform type.");

  viskores::cont::ArrayHandle<viskores::FloatDefault> fieldArray;
  this->DataSet.GetField(this->FieldName).GetData().AsArrayHandle(fieldArray);

  auto coords = this->DataSet.GetCoordinateSystem(0).GetData().AsArrayHandle<UniformCoordsType>();
  auto origin = coords.GetOrigin();
  auto spacing = coords.GetSpacing();
  auto dims = coords.GetDimensions();

  return viskores::exec::SplineEvaluateUniformGrid(
    origin, spacing, dims, fieldArray.PrepareForInput(device, token));
}

} //namespace cont
} //namespace viskores
