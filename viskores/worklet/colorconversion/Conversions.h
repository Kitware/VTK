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
#ifndef viskores_worklet_colorconversion_Conversions_h
#define viskores_worklet_colorconversion_Conversions_h

#include <viskores/Math.h>

#include <cmath>

namespace viskores
{
namespace worklet
{
namespace colorconversion
{

/// Cast the provided value to a `viskores::UInt8`. If the value is floating point,
/// it converts the range [0, 1] to [0, 255] (which is typical for how colors
/// are respectively represented in bytes and floats).
template <typename T>
VISKORES_EXEC inline viskores::UInt8 ColorToUChar(T t)
{
  return static_cast<viskores::UInt8>(t);
}

template <>
VISKORES_EXEC inline viskores::UInt8 ColorToUChar(viskores::Float64 t)
{
  return static_cast<viskores::UInt8>(std::round(t * 255.0f));
}

template <>
VISKORES_EXEC inline viskores::UInt8 ColorToUChar(viskores::Float32 t)
{
  return static_cast<viskores::UInt8>(std::round(t * 255.0f));
}


/// Clamp the provided value to the range [0, 255].
VISKORES_EXEC inline void Clamp(viskores::Float32& val)
{
  val = viskores::Min(255.0f, viskores::Max(0.0f, val));
}

// Note: due to a bug in Doxygen 1.8.17, we are not using the
// viskores::VecXT_X typedefs below.

/// Clamp the components of the provided value to the range [0, 255].
VISKORES_EXEC inline void Clamp(viskores::Vec<viskores::Float32, 2>& val)
{
  val[0] = viskores::Min(255.0f, viskores::Max(0.0f, val[0]));
  val[1] = viskores::Min(255.0f, viskores::Max(0.0f, val[1]));
}

/// Clamp the components of the provided value to the range [0, 255].
VISKORES_EXEC inline void Clamp(viskores::Vec<viskores::Float32, 3>& val)
{
  val[0] = viskores::Min(255.0f, viskores::Max(0.0f, val[0]));
  val[1] = viskores::Min(255.0f, viskores::Max(0.0f, val[1]));
  val[2] = viskores::Min(255.0f, viskores::Max(0.0f, val[2]));
}

/// Clamp the components of the provided value to the range [0, 255].
VISKORES_EXEC inline void Clamp(viskores::Vec<viskores::Float32, 4>& val)
{
  val[0] = viskores::Min(255.0f, viskores::Max(0.0f, val[0]));
  val[1] = viskores::Min(255.0f, viskores::Max(0.0f, val[1]));
  val[2] = viskores::Min(255.0f, viskores::Max(0.0f, val[2]));
  val[3] = viskores::Min(255.0f, viskores::Max(0.0f, val[3]));
}
}
}
}
#endif
