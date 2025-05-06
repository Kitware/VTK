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
//
//=============================================================================
#ifndef viskores_filter_ComputeMoments_hxx
#define viskores_filter_ComputeMoments_hxx

#include <viskores/cont/ArrayHandle.h>
#include <viskores/filter/image_processing/ComputeMoments.h>
#include <viskores/filter/image_processing/worklet/ComputeMoments.h>

namespace viskores
{
namespace filter
{
namespace image_processing
{

VISKORES_CONT ComputeMoments::ComputeMoments()
{
  this->SetOutputFieldName("moments_");
}

VISKORES_CONT viskores::cont::DataSet ComputeMoments::DoExecute(
  const viskores::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsPointField())
  {
    throw viskores::cont::ErrorBadValue("Active field for ComputeMoments must be a point field.");
  }

  viskores::cont::DataSet output = this->CreateResult(input);
  auto worklet = viskores::worklet::moments::ComputeMoments(this->Radius, this->Spacing);

  auto resolveType = [&](const auto& concrete)
  { worklet.Run(input.GetCellSet(), concrete, this->Order, output); };
  this->CastAndCallVariableVecField(field, resolveType);

  return output;
}
} // namespace image_processing
} // namespace filter
} // namespace viskores
#endif //viskores_filter_ComputeMoments_hxx
