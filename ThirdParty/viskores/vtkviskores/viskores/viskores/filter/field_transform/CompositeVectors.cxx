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

#include <viskores/filter/field_transform/CompositeVectors.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/List.h>
#include <viskores/TypeList.h>

namespace
{

// Extracts a component from an UnknownArrayHandle and returns the extracted component
// as an UnknownArrayHandle. Perhaps this functionality should be part of UnknownArrayHandle
// proper, but its use is probably rare. Note that this implementation makes some assumptions
// on its use in the CompositeVectors filter.
VISKORES_CONT viskores::cont::UnknownArrayHandle ExtractComponent(
  const viskores::cont::UnknownArrayHandle& array,
  viskores::IdComponent componentIndex)
{
  viskores::cont::UnknownArrayHandle extractedComponentArray;
  auto resolveType = [&](auto componentExample)
  {
    using ComponentType = decltype(componentExample);
    if (array.IsBaseComponentType<ComponentType>())
    {
      extractedComponentArray =
        array.ExtractComponent<ComponentType>(componentIndex, viskores::CopyFlag::Off);
    }
  };
  viskores::ListForEach(resolveType, viskores::TypeListBaseC{});
  return extractedComponentArray;
}

} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace field_transform
{

VISKORES_CONT void CompositeVectors::SetFieldNameList(
  const std::vector<std::string>& fieldNameList,
  viskores::cont::Field::Association association)
{
  viskores::IdComponent index = 0;
  for (auto& fieldName : fieldNameList)
  {
    this->SetActiveField(index, fieldName, association);
    ++index;
  }
}

VISKORES_CONT viskores::IdComponent CompositeVectors::GetNumberOfFields() const
{
  return this->GetNumberOfActiveFields();
}

VISKORES_CONT viskores::cont::DataSet CompositeVectors::DoExecute(
  const viskores::cont::DataSet& inDataSet)
{
  viskores::IdComponent numComponents = this->GetNumberOfFields();
  if (numComponents < 1)
  {
    throw viskores::cont::ErrorBadValue(
      "No input fields to combine into a vector for CompositeVectors.");
  }

  viskores::cont::UnknownArrayHandle outArray;

  // Allocate output array to the correct type.
  viskores::cont::Field firstField = this->GetFieldFromDataSet(0, inDataSet);
  viskores::Id numValues = firstField.GetNumberOfValues();
  viskores::cont::Field::Association association = firstField.GetAssociation();
  auto allocateOutput = [&](auto exampleComponent)
  {
    using ComponentType = decltype(exampleComponent);
    if (firstField.GetData().IsBaseComponentType<ComponentType>())
    {
      outArray = viskores::cont::ArrayHandleRuntimeVec<ComponentType>{ numComponents };
    }
  };
  viskores::ListForEach(allocateOutput, viskores::TypeListBaseC{});
  if (!outArray.IsValid() || (outArray.GetNumberOfComponentsFlat() != numComponents))
  {
    throw viskores::cont::ErrorBadType("Unable to allocate output array due to unexpected type.");
  }
  outArray.Allocate(numValues);

  // Iterate over all component fields and copy them into the output array.
  for (viskores::IdComponent componentIndex = 0; componentIndex < numComponents; ++componentIndex)
  {
    viskores::cont::Field inScalarField = this->GetFieldFromDataSet(componentIndex, inDataSet);
    if (inScalarField.GetData().GetNumberOfComponentsFlat() != 1)
    {
      throw viskores::cont::ErrorBadValue("All input fields to CompositeVectors must be scalars.");
    }
    if (inScalarField.GetAssociation() != association)
    {
      throw viskores::cont::ErrorBadValue(
        "All scalar fields must have the same association (point, cell, etc.).");
    }
    if (inScalarField.GetNumberOfValues() != numValues)
    {
      throw viskores::cont::ErrorBadValue("Inconsistent number of field values.");
    }

    ExtractComponent(outArray, componentIndex).DeepCopyFrom(inScalarField.GetData());
  }

  return this->CreateResultField(inDataSet, this->GetOutputFieldName(), association, outArray);
}

} // namespace field_transform
} // namespace viskores::filter
} // namespace viskores
