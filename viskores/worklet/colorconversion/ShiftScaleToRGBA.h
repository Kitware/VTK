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
#ifndef viskores_worklet_colorconversion_ShiftScaleToRGBA_h
#define viskores_worklet_colorconversion_ShiftScaleToRGBA_h

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/colorconversion/Conversions.h>

#include <viskores/Math.h>

namespace viskores
{
namespace worklet
{
namespace colorconversion
{

struct ShiftScaleToRGBA : public viskores::worklet::WorkletMapField
{
  const viskores::Float32 Shift;
  const viskores::Float32 Scale;
  const viskores::Float32 Alpha;

  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  ShiftScaleToRGBA(viskores::Float32 shift, viskores::Float32 scale, viskores::Float32 alpha)
    : WorkletMapField()
    , Shift(shift)
    , Scale(scale)
    , Alpha(alpha)
  {
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec4ui_8 operator()(const T& in) const
  { //vtkScalarsToColorsLuminanceToRGBA
    viskores::Float32 l = (static_cast<viskores::Float32>(in) + this->Shift) * this->Scale;
    colorconversion::Clamp(l);
    const viskores::UInt8 lc = static_cast<viskores::UInt8>(l + 0.5);
    return viskores::Vec4ui_8{ lc, lc, lc, colorconversion::ColorToUChar(this->Alpha) };
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec4ui_8 operator()(const viskores::Vec<T, 2>& in) const
  { //vtkScalarsToColorsLuminanceAlphaToRGBA
    viskores::Vec2f_32 la(in);
    la = (la + viskores::Vec2f_32(this->Shift)) * this->Scale;
    colorconversion::Clamp(la);

    const viskores::UInt8 lc = static_cast<viskores::UInt8>(la[0] + 0.5f);
    return viskores::Vec4ui_8{
      lc, lc, lc, static_cast<viskores::UInt8>((la[1] * this->Alpha) + 0.5f)
    };
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec4ui_8 operator()(const viskores::Vec<T, 3>& in) const
  { //vtkScalarsToColorsRGBToRGBA
    viskores::Vec3f_32 rgb(in);
    rgb = (rgb + viskores::Vec3f_32(this->Shift)) * this->Scale;
    colorconversion::Clamp(rgb);
    return viskores::Vec4ui_8{ static_cast<viskores::UInt8>(rgb[0] + 0.5f),
                               static_cast<viskores::UInt8>(rgb[1] + 0.5f),
                               static_cast<viskores::UInt8>(rgb[2] + 0.5f),
                               colorconversion::ColorToUChar(this->Alpha) };
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec4ui_8 operator()(const viskores::Vec<T, 4>& in) const
  { //vtkScalarsToColorsRGBAToRGBA
    viskores::Vec4f_32 rgba(in);
    rgba = (rgba + viskores::Vec4f_32(this->Shift)) * this->Scale;
    colorconversion::Clamp(rgba);

    rgba[3] *= this->Alpha;
    return viskores::Vec4ui_8{ static_cast<viskores::UInt8>(rgba[0] + 0.5f),
                               static_cast<viskores::UInt8>(rgba[1] + 0.5f),
                               static_cast<viskores::UInt8>(rgba[2] + 0.5f),
                               static_cast<viskores::UInt8>(rgba[3] + 0.5f) };
  }
};
}
}
}
#endif
