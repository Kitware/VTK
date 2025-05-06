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
#ifndef viskores_rendering_Color_h
#define viskores_rendering_Color_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <iostream>
#include <viskores/Types.h>
namespace viskores
{
namespace rendering
{

/// @brief Representation of a color.
///
/// The color is defined as red, green, and blue intensities as well as
/// an alpha representation of transparency (RGBA). The class provides
/// mechanisms to retrieve the color as 8-bit integers or floating point
/// values in the range [0, 1].
///
class Color
{
public:
  viskores::Vec4f_32 Components;

  /// @brief Create a black color.
  VISKORES_EXEC_CONT
  Color()
    : Components(0, 0, 0, 1)
  {
  }

  /// @brief Create a color with specified RGBA values.
  ///
  /// The values are floating point and in the range [0, 1].
  VISKORES_EXEC_CONT
  Color(viskores::Float32 r_,
        viskores::Float32 g_,
        viskores::Float32 b_,
        viskores::Float32 a_ = 1.f)
    : Components(r_, g_, b_, a_)
  {
  }

  /// @brief Create a color with specified RGBA values.
  ///
  /// The values are floating point and in the range [0, 1].
  VISKORES_EXEC_CONT
  Color(const viskores::Vec4f_32& components)
    : Components(components)
  {
  }

  /// @brief Set the color value from 8 bit RGBA components.
  ///
  /// The components are packed together into a 32-bit (4-byte) values.
  VISKORES_EXEC_CONT
  void SetComponentFromByte(viskores::Int32 i, viskores::UInt8 v)
  {
    // Note that though GetComponentAsByte below
    // multiplies by 256, we're dividing by 255. here.
    // This is, believe it or not, still correct.
    // That's partly because we always round down in
    // that method.  For example, if we set the float
    // here using byte(1), /255 gives us .00392, which
    // *256 gives us 1.0035, which is then rounded back
    // down to byte(1) below.  Or, if we set the float
    // here using byte(254), /255 gives us .99608, which
    // *256 gives us 254.996, which is then rounded
    // back down to 254 below.  So it actually reverses
    // correctly, even though the multiplier and
    // divider don't match between these two methods.
    //
    // Of course, converting in GetComponentAsByte from
    // 1.0 gives 256, so we need to still clamp to 255
    // anyway.  Again, this is not a problem, because it
    // doesn't really extend the range of floating point
    // values which map to 255.
    Components[i] = static_cast<viskores::Float32>(v) / 255.f;
    // clamp?
    if (Components[i] < 0)
      Components[i] = 0;
    if (Components[i] > 1)
      Components[i] = 1;
  }

  VISKORES_EXEC_CONT
  viskores::UInt8 GetComponentAsByte(int i)
  {
    // We need this to match what OpenGL/Mesa do.
    // Why?  Well, we need to set glClearColor
    // using floats, but the frame buffer comes
    // back as bytes (and is internally such) in
    // most cases.  In one example -- parallel
    // compositing -- we need the byte values
    // returned from here to match the byte values
    // returned in the frame buffer.  Though
    // a quick source code inspection of Mesa
    // led me to believe I should do *255., in
    // fact this led to a mismatch.  *256. was
    // actually closer.  (And arguably more correct
    // if you think the byte value 255 should share
    // approximately the same range in the float [0,1]
    // space as the other byte values.)  Note in the
    // inverse method above, though, we still use 255;
    // see SetComponentFromByte for an explanation of
    // why that is correct, if non-obvious.

    int tv = viskores::Int32(Components[i] * 256.f);
    // Converting even from valid values (i.e 1.0)
    // can give a result outside the range (i.e. 256),
    // but we have to clamp anyway.
    return viskores::UInt8((tv < 0) ? 0 : (tv > 255) ? 255 : tv);
  }

  VISKORES_EXEC_CONT
  void GetRGBA(viskores::UInt8& r, viskores::UInt8& g, viskores::UInt8& b, viskores::UInt8& a)
  {
    r = GetComponentAsByte(0);
    g = GetComponentAsByte(1);
    b = GetComponentAsByte(2);
    a = GetComponentAsByte(3);
  }

  VISKORES_EXEC_CONT
  viskores::Float64 RawBrightness() { return (Components[0] + Components[1] + Components[2]) / 3.; }

  VISKORES_CONT
  friend std::ostream& operator<<(std::ostream& out, const Color& c)
  {
    out << "[" << c.Components[0] << "," << c.Components[1] << "," << c.Components[2] << ","
        << c.Components[3] << "]";
    return out;
  }

  static VISKORES_RENDERING_EXPORT Color white, black;
  static VISKORES_RENDERING_EXPORT Color red, green, blue;
  static VISKORES_RENDERING_EXPORT Color cyan, magenta, yellow;
  static VISKORES_RENDERING_EXPORT Color gray10, gray20, gray30, gray40, gray50, gray60, gray70,
    gray80, gray90;
};
}
} //namespace viskores::rendering
#endif //viskores_rendering_Color_h
