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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayIsMonotonic.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/SplineEvaluateRectilinearGrid.h>

namespace viskores
{
namespace cont
{

viskores::exec::SplineEvaluateRectilinearGrid SplineEvaluateRectilinearGrid::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token) const
{
  using AxisType = viskores::cont::ArrayHandle<viskores::FloatDefault>;
  using RectCoordsType = viskores::cont::ArrayHandleCartesianProduct<AxisType, AxisType, AxisType>;

  if (!this->DataSet.GetCoordinateSystem(0).GetData().IsType<RectCoordsType>())
    throw viskores::cont::ErrorBadType("Coordinates are not rectilinear type.");

  viskores::cont::CoordinateSystem coordSystem = this->DataSet.GetCoordinateSystem();
  RectCoordsType coords = coordSystem.GetData().template AsArrayHandle<RectCoordsType>();
  if (!viskores::cont::ArrayIsMonotonicIncreasing(coords.GetFirstArray()) ||
      !viskores::cont::ArrayIsMonotonicIncreasing(coords.GetSecondArray()) ||
      !viskores::cont::ArrayIsMonotonicIncreasing(coords.GetThirdArray()))

  {
    throw viskores::cont::ErrorBadType("Coordinates are not monotonic increasing.");
  }

  viskores::cont::ArrayHandle<viskores::FloatDefault> fieldArray;
  viskores::cont::ArrayCopyShallowIfPossible(this->DataSet.GetField(this->FieldName).GetData(),
                                             fieldArray);

  return viskores::exec::SplineEvaluateRectilinearGrid(
    coords, fieldArray, coordSystem.GetBounds(), device, token);
}

} //namespace cont
} //namespace viskores
