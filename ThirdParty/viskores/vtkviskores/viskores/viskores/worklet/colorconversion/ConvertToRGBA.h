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
#ifndef viskores_worklet_colorconversion_ScalarsToColors_h
#define viskores_worklet_colorconversion_ScalarsToColors_h

#include <viskores/worklet/colorconversion/Conversions.h>

namespace viskores
{
namespace worklet
{
namespace colorconversion
{

struct ConvertToRGBA : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  ConvertToRGBA(viskores::Float32 alpha)
    : Alpha(alpha)
  {
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec4ui_8 operator()(const T& in) const
  { //vtkScalarsToColorsLuminanceToRGBA
    const viskores::UInt8 l = colorconversion::ColorToUChar(in);
    return viskores::Vec<UInt8, 4>(l, l, l, colorconversion::ColorToUChar(this->Alpha));
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec4ui_8 operator()(const viskores::Vec<T, 2>& in) const
  { //vtkScalarsToColorsLuminanceAlphaToRGBA
    const viskores::UInt8 l = colorconversion::ColorToUChar(in[0]);
    const viskores::UInt8 a = colorconversion::ColorToUChar(in[1]);
    return viskores::Vec<UInt8, 4>(l, l, l, static_cast<viskores::UInt8>(a * this->Alpha + 0.5f));
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec4ui_8 operator()(const viskores::Vec<T, 3>& in) const
  { //vtkScalarsToColorsRGBToRGBA
    return viskores::Vec<UInt8, 4>(colorconversion::ColorToUChar(in[0]),
                                   colorconversion::ColorToUChar(in[1]),
                                   colorconversion::ColorToUChar(in[2]),
                                   colorconversion::ColorToUChar(this->Alpha));
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec4ui_8 operator()(const viskores::Vec<T, 4>& in) const
  { //vtkScalarsToColorsRGBAToRGBA
    const viskores::UInt8 a = colorconversion::ColorToUChar(in[3]);
    return viskores::Vec<UInt8, 4>(colorconversion::ColorToUChar(in[0]),
                                   colorconversion::ColorToUChar(in[1]),
                                   colorconversion::ColorToUChar(in[2]),
                                   static_cast<viskores::UInt8>(a * this->Alpha + 0.5f));
  }

  const viskores::Float32 Alpha = 1.0f;
};
}
}
}
#endif
