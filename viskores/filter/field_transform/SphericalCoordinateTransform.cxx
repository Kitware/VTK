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

#include <viskores/cont/ErrorBadValue.h>
#include <viskores/filter/field_transform/SphericalCoordinateTransform.h>
#include <viskores/filter/field_transform/worklet/CoordinateSystemTransform.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{

SphericalCoordinateTransform::SphericalCoordinateTransform()
{
  this->SetUseCoordinateSystemAsField(true);
}

viskores::cont::DataSet SphericalCoordinateTransform::DoExecute(
  const viskores::cont::DataSet& inDataSet)
{
  viskores::cont::UnknownArrayHandle outArray;

  const viskores::cont::Field& inField = this->GetFieldFromDataSet(inDataSet);
  if (!inField.IsPointField())
  {
    throw viskores::cont::ErrorBadValue("SphericalCoordinateTransform only applies to point data.");
  }

  auto resolveType = [&](const auto& concrete)
  {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    viskores::cont::ArrayHandle<T> result;
    if (this->CartesianToSpherical)
      this->Invoke(viskores::worklet::CarToSphere{}, concrete, result);
    else
      this->Invoke(viskores::worklet::SphereToCar{}, concrete, result);
    outArray = result;
  };
  this->CastAndCallVecField<3>(inField, resolveType);

  std::string coordinateName = this->GetOutputFieldName();
  if (coordinateName.empty())
  {
    coordinateName = inField.GetName();
  }

  viskores::cont::DataSet outDataSet = this->CreateResultCoordinateSystem(
    inDataSet,
    inDataSet.GetCellSet(),
    viskores::cont::CoordinateSystem(coordinateName, outArray),
    [](viskores::cont::DataSet& out, const viskores::cont::Field& fieldToPass)
    { out.AddField(fieldToPass); });
  return outDataSet;
}
} // namespace field_transform
} // namespace filter
} // namespace viskores
