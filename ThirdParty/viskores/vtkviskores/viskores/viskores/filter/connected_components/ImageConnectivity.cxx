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

#include <viskores/filter/connected_components/ImageConnectivity.h>
#include <viskores/filter/connected_components/worklet/ImageConnectivity.h>

namespace viskores
{
namespace filter
{
namespace connected_components
{
VISKORES_CONT viskores::cont::DataSet ImageConnectivity::DoExecute(
  const viskores::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);

  if (!field.IsPointField())
  {
    throw viskores::cont::ErrorBadValue(
      "Active field for ImageConnectivity must be a point field.");
  }

  viskores::cont::ArrayHandle<viskores::Id> component;

  auto resolveType = [&](const auto& concrete)
  {
    viskores::worklet::connectivity::ImageConnectivity().Run(
      input.GetCellSet(), concrete, component);
  };
  this->CastAndCallScalarField(field, resolveType);

  return this->CreateResultFieldPoint(input, this->GetOutputFieldName(), component);
}
} // namespace connected_components
} // namespace filter
} // namespace viskores
