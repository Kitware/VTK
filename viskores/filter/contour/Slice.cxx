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

#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/filter/contour/Slice.h>

namespace viskores
{
namespace filter
{
namespace contour
{
viskores::cont::DataSet Slice::DoExecute(const viskores::cont::DataSet& input)
{
  const auto& coords = input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  viskores::cont::DataSet result;
  auto impFuncEval =
    viskores::ImplicitFunctionValueFunctor<viskores::ImplicitFunctionGeneral>(this->Function);
  auto coordTransform =
    viskores::cont::make_ArrayHandleTransform(coords.GetDataAsMultiplexer(), impFuncEval);
  viskores::cont::ArrayHandle<viskores::FloatDefault> sliceScalars;
  viskores::cont::ArrayCopyDevice(coordTransform, sliceScalars);
  // input is a const, we can not AddField to it.
  viskores::cont::DataSet clone = input;
  clone.AddField(viskores::cont::make_FieldPoint("sliceScalars", sliceScalars));

  if (this->GetNumberOfIsoValues() < 1)
  {
    this->SetIsoValue(0.0);
  }
  this->Contour::SetActiveField("sliceScalars");
  result = this->Contour::DoExecute(clone);

  return result;
}
} // namespace contour
} // namespace filter
} // namespace viskores
