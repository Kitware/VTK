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

#include <viskores/filter/vector_analysis/CrossProduct.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/ArrayCopy.h>

#include <viskores/VectorAnalysis.h>

namespace
{

class CrossProductWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);

  template <typename T>
  VISKORES_EXEC void operator()(const viskores::Vec<T, 3>& vec1,
                                const viskores::Vec<T, 3>& vec2,
                                viskores::Vec<T, 3>& outVec) const
  {
    outVec = viskores::Cross(vec1, vec2);
  }
};

} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace vector_analysis
{

//-----------------------------------------------------------------------------
VISKORES_CONT CrossProduct::CrossProduct()
{
  this->SetOutputFieldName("crossproduct");
}

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet CrossProduct::DoExecute(
  const viskores::cont::DataSet& inDataSet)
{
  viskores::cont::Field primaryField = this->GetFieldFromDataSet(0, inDataSet);
  viskores::cont::UnknownArrayHandle primaryArray = primaryField.GetData();

  viskores::cont::UnknownArrayHandle outArray;

  // We are using a C++14 auto lambda here. The advantage over a Functor is obvious, we don't
  // need to explicitly pass filter, input/output DataSets etc. thus reduce the impact to
  // the legacy code. The lambda can also access the private part of the filter thus reducing
  // filter's public interface profile. CastAndCall tries to cast primaryArray of unknown value
  // type and storage to a concrete ArrayHandle<T, S> with T from the `TypeList` and S from
  // `StorageList`. It then passes the concrete array to the lambda as the first argument.
  // We can later recover the concrete ValueType, T, from the concrete array.
  auto resolveType = [&](const auto& concrete)
  {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    const auto& secondaryField = this->GetFieldFromDataSet(1, inDataSet);
    viskores::cont::ArrayHandle<T> secondaryArray;
    viskores::cont::ArrayCopyShallowIfPossible(secondaryField.GetData(), secondaryArray);

    viskores::cont::ArrayHandle<T> result;
    this->Invoke(CrossProductWorklet{}, concrete, secondaryArray, result);
    outArray = result;
  };

  this->CastAndCallVecField<3>(primaryArray, resolveType);

  return this->CreateResultField(
    inDataSet, this->GetOutputFieldName(), primaryField.GetAssociation(), outArray);
}

}
}
} // namespace viskores::filter::vector_analysis
