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

#include <viskores/worklet/ScalarsToColors.h>

#include <viskores/VecTraits.h>
#include <viskores/cont/ArrayHandleExtractComponent.h>
#include <viskores/cont/ArrayHandleTransform.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/colorconversion/ConvertToRGB.h>
#include <viskores/worklet/colorconversion/ConvertToRGBA.h>
#include <viskores/worklet/colorconversion/Portals.h>
#include <viskores/worklet/colorconversion/ShiftScaleToRGB.h>
#include <viskores/worklet/colorconversion/ShiftScaleToRGBA.h>

namespace viskores
{
namespace worklet
{
namespace colorconversion
{
inline bool needShiftScale(viskores::Float32, viskores::Float32 shift, viskores::Float32 scale)
{
  return !((shift == -0.0f || shift == 0.0f) && (scale == 255.0f));
}
inline bool needShiftScale(viskores::Float64, viskores::Float32 shift, viskores::Float32 scale)
{
  return !((shift == -0.0f || shift == 0.0f) && (scale == 255.0f));
}
inline bool needShiftScale(viskores::UInt8, viskores::Float32 shift, viskores::Float32 scale)
{
  return !((shift == -0.0f || shift == 0.0f) && (scale == 1.0f));
}

template <typename T>
inline bool needShiftScale(T, viskores::Float32, viskores::Float32)
{
  return true;
}
}
/// \brief Use each component to generate RGBA colors
///
template <typename T, typename S>
void ScalarsToColors::Run(const viskores::cont::ArrayHandle<T, S>& values,
                          viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut) const
{
  using namespace viskores::worklet::colorconversion;
  //If our shift is 0 and our scale == 1 no need to apply them
  using BaseT = typename viskores::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    viskores::worklet::DispatcherMapField<ShiftScaleToRGBA> dispatcher(
      ShiftScaleToRGBA(this->Shift, this->Scale, this->Alpha));
    dispatcher.Invoke(values, rgbaOut);
  }
  else
  {
    viskores::worklet::DispatcherMapField<ConvertToRGBA> dispatcher(ConvertToRGBA(this->Alpha));
    dispatcher.Invoke(values, rgbaOut);
  }
}

/// \brief Use each component to generate RGB colors
///
template <typename T, typename S>
void ScalarsToColors::Run(const viskores::cont::ArrayHandle<T, S>& values,
                          viskores::cont::ArrayHandle<viskores::Vec3ui_8>& rgbOut) const
{
  using namespace viskores::worklet::colorconversion;
  using BaseT = typename viskores::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    viskores::worklet::DispatcherMapField<ShiftScaleToRGB> dispatcher(
      ShiftScaleToRGB(this->Shift, this->Scale));
    dispatcher.Invoke(values, rgbOut);
  }
  else
  {
    viskores::worklet::DispatcherMapField<ConvertToRGB> dispatcher;
    dispatcher.Invoke(values, rgbOut);
  }
}

/// \brief Use magnitude of a vector to generate RGBA colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunMagnitude(
  const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
  viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut) const
{
  //magnitude is a complex situation. the default scale factor is incorrect
  //
  using namespace viskores::worklet::colorconversion;
  //If our shift is 0 and our scale == 1 no need to apply them
  using BaseT = typename viskores::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    viskores::worklet::DispatcherMapField<ShiftScaleToRGBA> dispatcher(
      ShiftScaleToRGBA(this->Shift, this->Scale, this->Alpha));
    dispatcher.Invoke(
      viskores::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()),
      rgbaOut);
  }
  else
  {
    viskores::worklet::DispatcherMapField<ConvertToRGBA> dispatcher(ConvertToRGBA(this->Alpha));
    dispatcher.Invoke(
      viskores::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()),
      rgbaOut);
  }
}

/// \brief Use magnitude of a vector to generate RGB colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunMagnitude(
  const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
  viskores::cont::ArrayHandle<viskores::Vec3ui_8>& rgbOut) const
{

  using namespace viskores::worklet::colorconversion;
  using BaseT = typename viskores::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    viskores::worklet::DispatcherMapField<ShiftScaleToRGB> dispatcher(
      ShiftScaleToRGB(this->Shift, this->Scale));
    dispatcher.Invoke(
      viskores::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()),
      rgbOut);
  }
  else
  {
    viskores::worklet::DispatcherMapField<ConvertToRGB> dispatcher;
    dispatcher.Invoke(
      viskores::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()),
      rgbOut);
  }
}

/// \brief Use a single component of a vector to generate RGBA colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunComponent(
  const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
  viskores::IdComponent comp,
  viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut) const
{
  this->Run(
    viskores::cont::make_ArrayHandleTransform(values, colorconversion::ComponentPortal(comp)),
    rgbaOut);
}

/// \brief Use a single component of a vector to generate RGB colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunComponent(
  const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
  viskores::IdComponent comp,
  viskores::cont::ArrayHandle<viskores::Vec3ui_8>& rgbOut) const
{
  this->Run(
    viskores::cont::make_ArrayHandleTransform(values, colorconversion::ComponentPortal(comp)),
    rgbOut);
}
}
}
