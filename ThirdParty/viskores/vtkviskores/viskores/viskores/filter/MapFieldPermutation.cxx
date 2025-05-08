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

#include <viskores/cont/Logging.h>

#include <viskores/cont/internal/MapArrayPermutation.h>

#include <viskores/filter/MapFieldPermutation.h>

VISKORES_FILTER_CORE_EXPORT VISKORES_CONT bool viskores::filter::MapFieldPermutation(
  const viskores::cont::Field& inputField,
  const viskores::cont::ArrayHandle<viskores::Id>& permutation,
  viskores::cont::Field& outputField,
  viskores::Float64 invalidValue)
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  try
  {
    viskores::cont::UnknownArrayHandle outputArray = viskores::cont::internal::MapArrayPermutation(
      inputField.GetData(), permutation, invalidValue);
    outputField =
      viskores::cont::Field(inputField.GetName(), inputField.GetAssociation(), outputArray);
    return true;
  }
  catch (...)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn, "Faild to map field " << inputField.GetName());
    return false;
  }
}

VISKORES_FILTER_CORE_EXPORT VISKORES_CONT bool viskores::filter::MapFieldPermutation(
  const viskores::cont::CoordinateSystem& inputCoords,
  const viskores::cont::ArrayHandle<viskores::Id>& permutation,
  viskores::cont::CoordinateSystem& outputCoords,
  viskores::Float64 invalidValue)
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  try
  {
    viskores::cont::UnknownArrayHandle outputArray = viskores::cont::internal::MapArrayPermutation(
      inputCoords.GetData(), permutation, invalidValue);
    outputCoords =
      viskores::cont::Field(inputCoords.GetName(), inputCoords.GetAssociation(), outputArray);
    return true;
  }
  catch (...)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   "Faild to coordinate system " << inputCoords.GetName());
    return false;
  }
}

VISKORES_FILTER_CORE_EXPORT VISKORES_CONT bool viskores::filter::MapFieldPermutation(
  const viskores::cont::Field& inputField,
  const viskores::cont::ArrayHandle<viskores::Id>& permutation,
  viskores::cont::DataSet& outputData,
  viskores::Float64 invalidValue)
{
  viskores::cont::Field outputField;
  bool success =
    viskores::filter::MapFieldPermutation(inputField, permutation, outputField, invalidValue);
  if (success)
  {
    outputData.AddField(outputField);
  }
  return success;
}
