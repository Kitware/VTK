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

#include <viskores/filter/field_transform/PointTransform.h>
#include <viskores/filter/field_transform/worklet/PointTransform.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{
//-----------------------------------------------------------------------------
VISKORES_CONT PointTransform::PointTransform()
{
  this->SetOutputFieldName("transform");
  this->SetUseCoordinateSystemAsField(true);
}

//-----------------------------------------------------------------------------
VISKORES_CONT void PointTransform::SetChangeCoordinateSystem(bool flag)
{
  this->ChangeCoordinateSystem = flag;
}

//-----------------------------------------------------------------------------
VISKORES_CONT bool PointTransform::GetChangeCoordinateSystem() const
{
  return this->ChangeCoordinateSystem;
}

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet PointTransform::DoExecute(
  const viskores::cont::DataSet& inDataSet)
{
  viskores::cont::UnknownArrayHandle outArray;

  auto resolveType = [&](const auto& concrete)
  {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    viskores::cont::ArrayHandle<T> result;
    this->Invoke(viskores::worklet::PointTransform{ this->matrix }, concrete, result);
    outArray = result;
  };
  const auto& field = this->GetFieldFromDataSet(inDataSet);
  this->CastAndCallVecField<3>(field, resolveType);

  auto passMapper = [](auto& d, const auto& f) { d.AddField(f); };
  viskores::cont::DataSet outData = this->CreateResultCoordinateSystem(
    inDataSet, inDataSet.GetCellSet(), this->GetOutputFieldName(), outArray, passMapper);

  if (this->GetChangeCoordinateSystem())
  {
    viskores::Id coordIndex =
      this->GetUseCoordinateSystemAsField() ? this->GetActiveCoordinateSystemIndex() : 0;
    outData.GetCoordinateSystem(coordIndex).SetData(outArray);
  }

  return outData;
}
}
}
} // namespace viskores::filter
