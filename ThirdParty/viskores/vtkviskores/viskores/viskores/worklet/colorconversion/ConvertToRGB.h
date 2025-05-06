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
#ifndef viskores_worklet_colorconversion_ConvertToRGB_h
#define viskores_worklet_colorconversion_ConvertToRGB_h

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/colorconversion/Conversions.h>

namespace viskores
{
namespace worklet
{
namespace colorconversion
{

struct ConvertToRGB : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  template <typename T>
  VISKORES_EXEC viskores::Vec3ui_8 operator()(const T& in) const
  { //vtkScalarsToColorsLuminanceToRGB
    const viskores::UInt8 la = colorconversion::ColorToUChar(in);
    return viskores::Vec<UInt8, 3>(la, la, la);
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec3ui_8 operator()(const viskores::Vec<T, 2>& in) const
  { //vtkScalarsToColorsLuminanceAlphaToRGB (which actually doesn't exist in vtk)
    return this->operator()(in[0]);
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec3ui_8 operator()(const viskores::Vec<T, 3>& in) const
  { //vtkScalarsToColorsRGBToRGB
    return viskores::Vec<UInt8, 3>(colorconversion::ColorToUChar(in[0]),
                                   colorconversion::ColorToUChar(in[1]),
                                   colorconversion::ColorToUChar(in[2]));
  }

  VISKORES_EXEC viskores::Vec3ui_8 operator()(const viskores::Vec3ui_8& in) const
  { //vtkScalarsToColorsRGBToRGB
    return in;
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec3ui_8 operator()(const viskores::Vec<T, 4>& in) const
  { //vtkScalarsToColorsRGBAToRGB
    return viskores::Vec<UInt8, 3>(colorconversion::ColorToUChar(in[0]),
                                   colorconversion::ColorToUChar(in[1]),
                                   colorconversion::ColorToUChar(in[2]));
  }
};
}
}
}
#endif
