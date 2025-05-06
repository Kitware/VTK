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
#include <viskores/VecTraits.h>
#include <viskores/cont/ColorTableMap.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/field_transform/FieldToColors.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{
namespace
{
struct ScalarInputMode
{
};
struct MagnitudeInputMode
{
};
struct ComponentInputMode
{
};
}

template <typename T, typename S, typename U>
inline bool execute(ScalarInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    viskores::VecTraitsTagSingleComponent)
{
  return viskores::cont::ColorTableMap(input, samples, output);
}

template <typename T, typename S, typename U>
inline bool execute(MagnitudeInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    viskores::VecTraitsTagMultipleComponents)
{
  return viskores::cont::ColorTableMapMagnitude(input, samples, output);
}

template <typename T, typename S, typename U>
inline bool execute(ComponentInputMode,
                    int comp,
                    const T& input,
                    const S& samples,
                    U& output,
                    viskores::VecTraitsTagMultipleComponents)
{
  return viskores::cont::ColorTableMapComponent(input, comp, samples, output);
}

//error cases
template <typename T, typename S, typename U>
inline bool execute(ScalarInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    viskores::VecTraitsTagMultipleComponents)
{ //vector input in scalar mode so do magnitude
  return viskores::cont::ColorTableMapMagnitude(input, samples, output);
}
template <typename T, typename S, typename U>
inline bool execute(MagnitudeInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    viskores::VecTraitsTagSingleComponent)
{ //is a scalar array so ignore Magnitude mode
  return viskores::cont::ColorTableMap(input, samples, output);
}
template <typename T, typename S, typename U>
inline bool execute(ComponentInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    viskores::VecTraitsTagSingleComponent)
{ //is a scalar array so ignore InputMode::Component
  return viskores::cont::ColorTableMap(input, samples, output);
}


//-----------------------------------------------------------------------------
VISKORES_CONT FieldToColors::FieldToColors(const viskores::cont::ColorTable& table)
  : Table(table)

{
}

//-----------------------------------------------------------------------------
VISKORES_CONT void FieldToColors::SetNumberOfSamplingPoints(viskores::Int32 count)
{
  if (this->SampleCount != count && count > 0)
  {
    this->ModifiedCount = -1;
    this->SampleCount = count;
  }
}

//-----------------------------------------------------------------------------
viskores::cont::DataSet FieldToColors::DoExecute(const viskores::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);

  //If the table has been modified we need to rebuild our
  //sample tables
  if (this->Table.GetModifiedCount() > this->ModifiedCount)
  {
    this->Table.Sample(this->SampleCount, this->SamplesRGB);
    this->Table.Sample(this->SampleCount, this->SamplesRGBA);
    this->ModifiedCount = this->Table.GetModifiedCount();
  }

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    // Default name is name of input_colors.
    outputName = field.GetName() + "_colors";
  }
  viskores::cont::Field outField;

  //We need to verify if the array is a viskores::Vec
  viskores::cont::UnknownArrayHandle outArray;
  auto resolveType = [&](const auto& concrete)
  {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    using IsVec = typename viskores::VecTraits<T>::HasMultipleComponents;

    if (this->OutputModeType == OutputMode::RGBA)
    {
      viskores::cont::ArrayHandle<viskores::Vec4ui_8> result;

      bool ran = false;
      switch (this->InputModeType)
      {
        case InputMode::Scalar:
        {
          ran = execute(
            ScalarInputMode{}, this->Component, concrete, this->SamplesRGBA, result, IsVec{});
          break;
        }
        case InputMode::Magnitude:
        {
          ran = execute(
            MagnitudeInputMode{}, this->Component, concrete, this->SamplesRGBA, result, IsVec{});
          break;
        }
        case InputMode::Component:
        {
          ran = execute(
            ComponentInputMode{}, this->Component, concrete, this->SamplesRGBA, result, IsVec{});
          break;
        }
      }

      if (!ran)
      {
        throw viskores::cont::ErrorFilterExecution("Unsupported input mode.");
      }
      outField = viskores::cont::make_FieldPoint(outputName, result);
    }
    else
    {
      viskores::cont::ArrayHandle<viskores::Vec3ui_8> result;

      bool ran = false;
      switch (this->InputModeType)
      {
        case InputMode::Scalar:
        {
          ran = execute(
            ScalarInputMode{}, this->Component, concrete, this->SamplesRGB, result, IsVec{});
          break;
        }
        case InputMode::Magnitude:
        {
          ran = execute(
            MagnitudeInputMode{}, this->Component, concrete, this->SamplesRGB, result, IsVec{});
          break;
        }
        case InputMode::Component:
        {
          ran = execute(
            ComponentInputMode{}, this->Component, concrete, this->SamplesRGB, result, IsVec{});
          break;
        }
      }

      if (!ran)
      {
        throw viskores::cont::ErrorFilterExecution("Unsupported input mode.");
      }
      outField = viskores::cont::make_FieldPoint(outputName, result);
    }
  };
  field.GetData()
    .CastAndCallForTypesWithFloatFallback<viskores::TypeListField, VISKORES_DEFAULT_STORAGE_LIST>(
      resolveType);

  return this->CreateResultField(input, outField);
}
} // namespace field_transform
} // namespace filter
} // namespace viskores
