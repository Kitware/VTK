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
#ifndef viskores_worklet_colorconversion_ShiftScaleToRGB_h
#define viskores_worklet_colorconversion_ShiftScaleToRGB_h

#include <viskores/worklet/colorconversion/Conversions.h>

#include <viskores/Math.h>

namespace viskores
{
namespace worklet
{
namespace colorconversion
{

struct ShiftScaleToRGB : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  ShiftScaleToRGB(viskores::Float32 shift, viskores::Float32 scale)
    : Shift(shift)
    , Scale(scale)
  {
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec3ui_8 operator()(const T& in) const
  { //vtkScalarsToColorsLuminanceToRGB
    viskores::Float32 l = (static_cast<viskores::Float32>(in) + this->Shift) * this->Scale;
    colorconversion::Clamp(l);
    return viskores::Vec3ui_8{ static_cast<viskores::UInt8>(l + 0.5f) };
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec3ui_8 operator()(const viskores::Vec<T, 2>& in) const
  { //vtkScalarsToColorsLuminanceAlphaToRGB (which actually doesn't exist in vtk)
    return this->operator()(in[0]);
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec3ui_8 operator()(const viskores::Vec<T, 3>& in) const
  { //vtkScalarsToColorsRGBToRGB
    viskores::Vec3f_32 rgb(in);
    rgb = (rgb + viskores::Vec3f_32(this->Shift)) * this->Scale;
    colorconversion::Clamp(rgb);
    return viskores::Vec3ui_8{ static_cast<viskores::UInt8>(rgb[0] + 0.5f),
                               static_cast<viskores::UInt8>(rgb[1] + 0.5f),
                               static_cast<viskores::UInt8>(rgb[2] + 0.5f) };
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec3ui_8 operator()(const viskores::Vec<T, 4>& in) const
  { //vtkScalarsToColorsRGBAToRGB
    return this->operator()(viskores::Vec<T, 3>{ in[0], in[1], in[2] });
  }

private:
  const viskores::Float32 Shift;
  const viskores::Float32 Scale;
};
}
}
}
#endif
