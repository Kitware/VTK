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
#include <viskores/filter/vector_analysis/DotProduct.h>
#include <viskores/worklet/WorkletMapField.h>

namespace // anonymous namespace making worklet::DotProduct internal to this .cxx
{

struct DotProductWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);

  template <typename T1, typename T2, typename T3>
  VISKORES_EXEC void operator()(const T1& v1, const T2& v2, T3& outValue) const
  {
    VISKORES_ASSERT(v1.GetNumberOfComponents() == v2.GetNumberOfComponents());
    outValue = v1[0] * v2[0];
    for (viskores::IdComponent i = 1; i < v1.GetNumberOfComponents(); ++i)
    {
      outValue += v1[i] * v2[i];
    }
  }
};

template <typename PrimaryArrayType>
viskores::cont::UnknownArrayHandle DoDotProduct(const PrimaryArrayType& primaryArray,
                                                const viskores::cont::Field& secondaryField)
{
  using T = typename PrimaryArrayType::ValueType::ComponentType;

  viskores::cont::Invoker invoke;
  viskores::cont::ArrayHandle<T> outputArray;

  if (secondaryField.GetData().IsBaseComponentType<T>())
  {
    invoke(DotProductWorklet{},
           primaryArray,
           secondaryField.GetData().ExtractArrayFromComponents<T>(),
           outputArray);
  }
  else
  {
    // Data types of primary and secondary array do not match. Rather than try to replicate every
    // possibility, get the secondary array as a FloatDefault.
    viskores::cont::UnknownArrayHandle castSecondaryArray = secondaryField.GetDataAsDefaultFloat();
    invoke(DotProductWorklet{},
           primaryArray,
           castSecondaryArray.ExtractArrayFromComponents<viskores::FloatDefault>(),
           outputArray);
  }

  return outputArray;
}

} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace vector_analysis
{

VISKORES_CONT DotProduct::DotProduct()
{
  this->SetOutputFieldName("dotproduct");
}

VISKORES_CONT viskores::cont::DataSet DotProduct::DoExecute(
  const viskores::cont::DataSet& inDataSet)
{
  viskores::cont::Field primaryField = this->GetFieldFromDataSet(0, inDataSet);
  viskores::cont::Field secondaryField = this->GetFieldFromDataSet(1, inDataSet);

  if (primaryField.GetData().GetNumberOfComponentsFlat() !=
      secondaryField.GetData().GetNumberOfComponentsFlat())
  {
    throw viskores::cont::ErrorFilterExecution(
      "Primary and secondary arrays of DotProduct filter have different number of components.");
  }

  viskores::cont::UnknownArrayHandle outArray;

  auto resolveArray = [&](const auto& concretePrimaryField)
  { outArray = DoDotProduct(concretePrimaryField, secondaryField); };
  this->CastAndCallVariableVecField(primaryField, resolveArray);

  return this->CreateResultField(inDataSet,
                                 this->GetOutputFieldName(),
                                 this->GetFieldFromDataSet(inDataSet).GetAssociation(),
                                 outArray);
}

} // namespace vector_analysis
} // namespace filter
} // namespace viskores
