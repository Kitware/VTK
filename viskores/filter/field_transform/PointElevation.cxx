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

#include <viskores/filter/field_transform/PointElevation.h>
#include <viskores/filter/field_transform/worklet/PointElevation.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{
//-----------------------------------------------------------------------------
VISKORES_CONT PointElevation::PointElevation()
{
  this->SetOutputFieldName("elevation");
}

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet PointElevation::DoExecute(
  const viskores::cont::DataSet& inDataSet)
{
  viskores::cont::ArrayHandle<viskores::Float64> outArray;

  auto resolveType = [&](const auto& concrete)
  {
    this->Invoke(
      viskores::worklet::PointElevation{
        this->LowPoint, this->HighPoint, this->RangeLow, this->RangeHigh },
      concrete,
      outArray);
  };
  const auto& field = this->GetFieldFromDataSet(inDataSet);
  this->CastAndCallVecField<3>(field, resolveType);

  return this->CreateResultField(
    inDataSet, this->GetOutputFieldName(), field.GetAssociation(), outArray);
}
} // namespace field_transform
} // namespace filter
} // namespace viskores
