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

#include <viskores/filter/vector_analysis/VectorMagnitude.h>
#include <viskores/filter/vector_analysis/worklet/Magnitude.h>

namespace viskores
{
namespace filter
{
namespace vector_analysis
{
VectorMagnitude::VectorMagnitude()
{
  this->SetOutputFieldName("magnitude");
}

VISKORES_CONT viskores::cont::DataSet VectorMagnitude::DoExecute(
  const viskores::cont::DataSet& inDataSet)
{
  viskores::cont::UnknownArrayHandle outArray;

  auto resolveType = [&](const auto& concrete)
  {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
    using ReturnType = typename ::viskores::detail::FloatingPointReturnType<T>::Type;
    viskores::cont::ArrayHandle<ReturnType> result;

    this->Invoke(viskores::worklet::Magnitude{}, concrete, result);
    outArray = result;
  };
  const auto& field = this->GetFieldFromDataSet(inDataSet);
  field.GetData().CastAndCallWithExtractedArray(resolveType);

  return this->CreateResultField(
    inDataSet, this->GetOutputFieldName(), field.GetAssociation(), outArray);
}
} // namespace vector_analysis
} // namespace filter
} // namespace viskores
