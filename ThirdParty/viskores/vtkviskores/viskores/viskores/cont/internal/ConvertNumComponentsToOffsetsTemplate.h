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
#ifndef viskores_cont_internal_ConvertNumComponentsToOffsetsTemplate_h
#define viskores_cont_internal_ConvertNumComponentsToOffsetsTemplate_h


#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayGetValues.h>

namespace viskores
{
namespace cont
{
namespace internal
{

/// @{
/// \brief Template implementation of `ConvertNumComponentsToOffsets`.
///
/// This form of the function can be used in situations where the precompiled
/// `ConvertNumComponentsToOffsets` does not include code paths for a desired
/// array.
///
template <typename NumComponentsArrayType, typename OffsetsStorage>
VISKORES_CONT void ConvertNumComponentsToOffsetsTemplate(
  const NumComponentsArrayType& numComponentsArray,
  viskores::cont::ArrayHandle<viskores::Id, OffsetsStorage>& offsetsArray,
  viskores::Id& componentsArraySize,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny())
{
  using namespace viskores::cont;
  VISKORES_IS_ARRAY_HANDLE(NumComponentsArrayType);

  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  Algorithm::ScanExtended(
    device, make_ArrayHandleCast<viskores::Id>(numComponentsArray), offsetsArray);

  componentsArraySize =
    viskores::cont::ArrayGetValue(offsetsArray.GetNumberOfValues() - 1, offsetsArray);
}

template <typename NumComponentsArrayType, typename OffsetsStorage>
VISKORES_CONT void ConvertNumComponentsToOffsetsTemplate(
  const NumComponentsArrayType& numComponentsArray,
  viskores::cont::ArrayHandle<viskores::Id, OffsetsStorage>& offsetsArray,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny())
{
  VISKORES_IS_ARRAY_HANDLE(NumComponentsArrayType);

  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  viskores::cont::Algorithm::ScanExtended(
    device, viskores::cont::make_ArrayHandleCast<viskores::Id>(numComponentsArray), offsetsArray);
}

template <typename NumComponentsArrayType>
VISKORES_CONT viskores::cont::ArrayHandle<viskores::Id> ConvertNumComponentsToOffsetsTemplate(
  const NumComponentsArrayType& numComponentsArray,
  viskores::Id& componentsArraySize,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny())
{
  VISKORES_IS_ARRAY_HANDLE(NumComponentsArrayType);

  viskores::cont::ArrayHandle<viskores::Id> offsetsArray;
  viskores::cont::internal::ConvertNumComponentsToOffsetsTemplate(
    numComponentsArray, offsetsArray, componentsArraySize, device);
  return offsetsArray;
}

template <typename NumComponentsArrayType>
VISKORES_CONT viskores::cont::ArrayHandle<viskores::Id> ConvertNumComponentsToOffsetsTemplate(
  const NumComponentsArrayType& numComponentsArray,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny())
{
  VISKORES_IS_ARRAY_HANDLE(NumComponentsArrayType);

  viskores::cont::ArrayHandle<viskores::Id> offsetsArray;
  viskores::cont::internal::ConvertNumComponentsToOffsetsTemplate(
    numComponentsArray, offsetsArray, device);
  return offsetsArray;
}


/// @}

} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores

#endif // viskores_cont_internal_ConvertNumComponentsToOffsetsTemplate_h
