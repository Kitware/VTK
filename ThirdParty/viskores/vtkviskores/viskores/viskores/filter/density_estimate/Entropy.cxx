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
#include <viskores/filter/density_estimate/Entropy.h>
#include <viskores/filter/density_estimate/worklet/FieldEntropy.h>

namespace viskores
{
namespace filter
{
namespace density_estimate
{
//-----------------------------------------------------------------------------
VISKORES_CONT Entropy::Entropy()

{
  this->SetOutputFieldName("entropy");
}

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet Entropy::DoExecute(const viskores::cont::DataSet& inDataSet)
{
  viskores::worklet::FieldEntropy worklet;

  viskores::Float64 e = 0;
  auto resolveType = [&](const auto& concrete) { e = worklet.Run(concrete, this->NumberOfBins); };
  const auto& fieldArray = this->GetFieldFromDataSet(inDataSet).GetData();
  fieldArray.CastAndCallForTypesWithFloatFallback<viskores::TypeListFieldScalar,
                                                  VISKORES_DEFAULT_STORAGE_LIST>(resolveType);

  //the entropy vector only contain one element, the entropy of the input field
  viskores::cont::ArrayHandle<viskores::Float64> entropy;
  entropy.Allocate(1);
  entropy.WritePortal().Set(0, e);

  viskores::cont::DataSet output;
  output.AddField(
    { this->GetOutputFieldName(), viskores::cont::Field::Association::WholeDataSet, entropy });

  // The output is a "summary" of the input, no need to map fields
  return output;
}
} // namespace density_estimate
} // namespace filter
} // namespace viskores
