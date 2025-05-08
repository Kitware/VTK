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
#ifndef viskores_worklet_colorconversion_TransferFunction_h
#define viskores_worklet_colorconversion_TransferFunction_h

#include <viskores/exec/ColorTable.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/colorconversion/Conversions.h>

namespace viskores
{
namespace worklet
{
namespace colorconversion
{

struct TransferFunction : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, ExecObject colorTable, FieldOut color);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename T>
  VISKORES_EXEC void operator()(const T& in,
                                const viskores::exec::ColorTable& colorTable,
                                viskores::Vec3ui_8& output) const
  {
    viskores::Vec<float, 3> rgb = colorTable.MapThroughColorSpace(static_cast<double>(in));
    output[0] = colorconversion::ColorToUChar(rgb[0]);
    output[1] = colorconversion::ColorToUChar(rgb[1]);
    output[2] = colorconversion::ColorToUChar(rgb[2]);
  }

  template <typename T>
  VISKORES_EXEC void operator()(const T& in,
                                const viskores::exec::ColorTable& colorTable,
                                viskores::Vec4ui_8& output) const
  {
    viskores::Vec<float, 3> rgb = colorTable.MapThroughColorSpace(static_cast<double>(in));
    float alpha = colorTable.MapThroughOpacitySpace(static_cast<double>(in));
    output[0] = colorconversion::ColorToUChar(rgb[0]);
    output[1] = colorconversion::ColorToUChar(rgb[1]);
    output[2] = colorconversion::ColorToUChar(rgb[2]);
    output[3] = colorconversion::ColorToUChar(alpha);
  }

  template <typename T>
  VISKORES_EXEC void operator()(const T& in,
                                const viskores::exec::ColorTable& colorTable,
                                viskores::Vec3f_32& output) const
  {
    output = colorTable.MapThroughColorSpace(static_cast<double>(in));
  }

  template <typename T>
  VISKORES_EXEC void operator()(const T& in,
                                const viskores::exec::ColorTable& colorTable,
                                viskores::Vec4f_32& output) const
  {
    viskores::Vec3f_32 rgb = colorTable.MapThroughColorSpace(static_cast<viskores::Float64>(in));
    viskores::Float32 alpha = colorTable.MapThroughOpacitySpace(static_cast<viskores::Float64>(in));
    output[0] = rgb[0];
    output[1] = rgb[1];
    output[2] = rgb[2];
    output[3] = alpha;
  }
};
}
}
}
#endif
