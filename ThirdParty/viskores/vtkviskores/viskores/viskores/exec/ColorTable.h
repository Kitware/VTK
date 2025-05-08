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
#ifndef viskores_exec_ColorTable_h
#define viskores_exec_ColorTable_h

#include <viskores/Types.h>

namespace viskores
{

enum struct ColorSpace
{
  RGB,
  HSV,
  HSVWrap,
  Lab,
  Diverging
};

} // namespace viskores

namespace viskores
{
namespace exec
{

class VISKORES_ALWAYS_EXPORT ColorTable
{
public:
  inline VISKORES_EXEC viskores::Vec3f_32 MapThroughColorSpace(viskores::Float64) const;

  inline VISKORES_EXEC viskores::Vec3f_32 MapThroughColorSpace(const viskores::Vec3f_32& rgb1,
                                                               const viskores::Vec3f_32& rgb2,
                                                               viskores::Float32 weight) const;

  inline VISKORES_EXEC viskores::Float32 MapThroughOpacitySpace(viskores::Float64 value) const;

  viskores::ColorSpace Space;

  viskores::Float64 const* ColorNodes = nullptr;
  viskores::Vec3f_32 const* RGB = nullptr;

  viskores::Float64 const* ONodes = nullptr;
  viskores::Float32 const* Alpha = nullptr;
  viskores::Vec2f_32 const* MidSharp = nullptr;

  viskores::Int32 ColorSize = 0;
  viskores::Int32 OpacitySize = 0;

  viskores::Vec3f_32 NaNColor = { 0.5f, 0.0f, 0.0f };
  viskores::Vec3f_32 BelowRangeColor = { 0.0f, 0.0f, 0.0f };
  viskores::Vec3f_32 AboveRangeColor = { 0.0f, 0.0f, 0.0f };

  bool UseClamping = true;

private:
  inline VISKORES_EXEC void FindColors(viskores::Float64 value,
                                       viskores::Vec3f_32& first,
                                       viskores::Vec3f_32& second,
                                       viskores::Float32& weight) const;

  inline VISKORES_EXEC viskores::Vec3f_32 MapThroughColorSpaceRGB(const viskores::Vec3f_32& rgb1,
                                                                  const viskores::Vec3f_32& rgb2,
                                                                  viskores::Float32 weight) const;

  inline VISKORES_EXEC viskores::Vec3f_32 MapThroughColorSpaceHSV(const viskores::Vec3f_32& rgb1,
                                                                  const viskores::Vec3f_32& rgb2,
                                                                  viskores::Float32 weight) const;

  inline VISKORES_EXEC viskores::Vec3f_32 MapThroughColorSpaceHSVWrap(
    const viskores::Vec3f_32& rgb1,
    const viskores::Vec3f_32& rgb2,
    viskores::Float32 weight) const;

  inline VISKORES_EXEC viskores::Vec3f_32 MapThroughColorSpaceLab(const viskores::Vec3f_32& rgb1,
                                                                  const viskores::Vec3f_32& rgb2,
                                                                  viskores::Float32 weight) const;

  inline VISKORES_EXEC viskores::Vec3f_32 MapThroughColorSpaceDiverging(
    const viskores::Vec3f_32& rgb1,
    const viskores::Vec3f_32& rgb2,
    viskores::Float32 weight) const;
};

}
}

#include <viskores/exec/ColorTable.hxx>

#endif
