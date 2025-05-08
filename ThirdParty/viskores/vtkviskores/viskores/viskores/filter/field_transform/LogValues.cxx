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
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/field_transform/LogValues.h>
#include <viskores/filter/field_transform/worklet/LogValues.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{
VISKORES_CONT viskores::cont::DataSet LogValues::DoExecute(const viskores::cont::DataSet& inDataSet)
{
  viskores::cont::ArrayHandle<viskores::FloatDefault> logField;
  auto resolveType = [&](const auto& concrete)
  {
    switch (this->BaseValue)
    {
      case LogBase::E:
      {
        this->Invoke(viskores::worklet::detail::LogFunWorklet<viskores::Log>{ this->GetMinValue() },
                     concrete,
                     logField);
        break;
      }
      case LogBase::TWO:
      {
        this->Invoke(
          viskores::worklet::detail::LogFunWorklet<viskores::Log2>{ this->GetMinValue() },
          concrete,
          logField);
        break;
      }
      case LogBase::TEN:
      {
        this->Invoke(
          viskores::worklet::detail::LogFunWorklet<viskores::Log10>{ this->GetMinValue() },
          concrete,
          logField);
        break;
      }
      default:
      {
        throw viskores::cont::ErrorFilterExecution("Unsupported base value.");
      }
    }
  };
  const auto& inField = this->GetFieldFromDataSet(inDataSet);
  this->CastAndCallScalarField(inField, resolveType);

  return this->CreateResultField(
    inDataSet, this->GetOutputFieldName(), inField.GetAssociation(), logField);
}
} // namespace field_transform
} // namespace viskores::filter
} // namespace viskores
