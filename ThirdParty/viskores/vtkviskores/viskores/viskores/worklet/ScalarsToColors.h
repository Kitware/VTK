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
#ifndef viskores_worklet_ScalarsToColors_h
#define viskores_worklet_ScalarsToColors_h

#include <viskores/Range.h>
#include <viskores/cont/ArrayHandle.h>

namespace viskores
{
namespace worklet
{

namespace colorconversion
{
inline void ComputeShiftScale(const viskores::Range& range,
                              viskores::Float32& shift,
                              viskores::Float32& scale)
{
  //This scale logic seems to be unduly complicated
  shift = static_cast<viskores::Float32>(-range.Min);
  scale = static_cast<viskores::Float32>(range.Length());

  if (range.Length() <= 0)
  {
    scale = -1e17f;
  }
  if (scale * scale > 1e-30f)
  {
    scale = 1.0f / scale;
  }
  scale *= 255.0f;
}
}

class ScalarsToColors
{
  viskores::Range ValueRange = { 0.0f, 255.0f };
  viskores::Float32 Alpha = 1.0f;
  viskores::Float32 Shift = 0.0f;
  viskores::Float32 Scale = 1.0f;

public:
  ScalarsToColors() {}

  ScalarsToColors(const viskores::Range& range, viskores::Float32 alpha)
    : ValueRange(range)
    , Alpha(viskores::Min(viskores::Max(alpha, 0.0f), 1.0f))
  {
    colorconversion::ComputeShiftScale(range, this->Shift, this->Scale);
  }

  ScalarsToColors(const viskores::Range& range)
    : ValueRange(range)
  {
    colorconversion::ComputeShiftScale(range, this->Shift, this->Scale);
  }

  ScalarsToColors(viskores::Float32 alpha)
    : ValueRange(0.0f, 255.0f)
    , Alpha(viskores::Min(viskores::Max(alpha, 0.0f), 1.0f))
  {
  }

  void SetRange(const viskores::Range& range)
  {
    this->ValueRange = range;
    colorconversion::ComputeShiftScale(range, this->Shift, this->Scale);
  }

  viskores::Range GetRange() const { return this->ValueRange; }

  void SetAlpha(viskores::Float32 alpha)
  {
    this->Alpha = viskores::Min(viskores::Max(alpha, 0.0f), 1.0f);
  }

  viskores::Float32 GetAlpha() const { return this->Alpha; }

  /// \brief Use each component to generate RGBA colors
  ///
  template <typename T, typename S>
  void Run(const viskores::cont::ArrayHandle<T, S>& values,
           viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut) const;

  /// \brief Use each component to generate RGB colors
  ///
  template <typename T, typename S>
  void Run(const viskores::cont::ArrayHandle<T, S>& values,
           viskores::cont::ArrayHandle<viskores::Vec3ui_8>& rgbOut) const;


  /// \brief Use magnitude of a vector to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  void RunMagnitude(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                    viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut) const;

  /// \brief Use magnitude of a vector to generate RGB colors
  ///
  template <typename T, int N, typename S>
  void RunMagnitude(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                    viskores::cont::ArrayHandle<viskores::Vec3ui_8>& rgbOut) const;

  /// \brief Use a single component of a vector to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  void RunComponent(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                    viskores::IdComponent comp,
                    viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut) const;

  /// \brief Use a single component of a vector to generate RGB colors
  ///
  template <typename T, int N, typename S>
  void RunComponent(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                    viskores::IdComponent comp,
                    viskores::cont::ArrayHandle<viskores::Vec3ui_8>& rgbOut) const;
};
}
}

#include <viskores/worklet/ScalarsToColors.hxx>

#endif
