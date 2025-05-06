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
#ifndef viskores_io_PixelTypes_hxx
#define viskores_io_PixelTypes_hxx

#include <viskores/Math.h>
#include <viskores/io/PixelTypes.h>

namespace viskores
{
namespace io
{

template <const viskores::Id B, const viskores::IdComponent C>
void BasePixel<B, C>::FillImageAtIndexWithPixel(unsigned char* imageData, const viskores::Id index)
{
  viskores::Id initShift = BIT_DEPTH - 8;
  for (viskores::Id channel = 0; channel < NUM_CHANNELS; channel++)
  {
    for (viskores::Id shift = initShift, i = 0; shift >= 0; shift -= 8, i++)
    {
      imageData[index * BYTES_PER_PIXEL + i + (channel * NUM_BYTES)] =
        static_cast<unsigned char>((this->Components[channel] & (0xff << shift)) >> shift);
    }
  }
}

template <const viskores::Id B, const viskores::IdComponent C>
void BasePixel<B, C>::ConstructPixelFromImage(const unsigned char* imageData,
                                              const viskores::Id index)
{
  viskores::Id initShift = BIT_DEPTH - 8;
  for (viskores::Id channel = 0; channel < NUM_CHANNELS; channel++)
  {
    for (viskores::Id shift = initShift, i = 0; shift >= 0; shift -= 8, i++)
    {
      this->Components[channel] |= imageData[index * BYTES_PER_PIXEL + i + (channel * NUM_BYTES)]
        << shift;
    }
  }
}

template <const viskores::Id B>
typename RGBPixel<B>::ComponentType RGBPixel<B>::Diff(const Superclass& pixel) const
{
  return static_cast<RGBPixel<B>::ComponentType>(viskores::Abs(this->Components[0] - pixel[0]) +
                                                 viskores::Abs(this->Components[1] - pixel[1]) +
                                                 viskores::Abs(this->Components[2] - pixel[2]));
}

template <const viskores::Id B>
viskores::Vec4f_32 RGBPixel<B>::ToVec4f() const
{
  return viskores::Vec4f_32(
    static_cast<viskores::Float32>(this->Components[0]) / this->MAX_COLOR_VALUE,
    static_cast<viskores::Float32>(this->Components[1]) / this->MAX_COLOR_VALUE,
    static_cast<viskores::Float32>(this->Components[2]) / this->MAX_COLOR_VALUE,
    1);
}

template <const viskores::Id B>
typename GreyPixel<B>::ComponentType GreyPixel<B>::Diff(const Superclass& pixel) const
{
  return static_cast<GreyPixel<B>::ComponentType>(viskores::Abs(this->Components[0] - pixel[0]));
}

template <const viskores::Id B>
viskores::Vec4f_32 GreyPixel<B>::ToVec4f() const
{
  return viskores::Vec4f_32(
    static_cast<viskores::Float32>(this->Components[0]) / this->MAX_COLOR_VALUE,
    static_cast<viskores::Float32>(this->Components[0]) / this->MAX_COLOR_VALUE,
    static_cast<viskores::Float32>(this->Components[0]) / this->MAX_COLOR_VALUE,
    1);
}

template <const viskores::Id B>
int RGBPixel<B>::GetColorType()
{
  return internal::RGBColorType;
}

template <const viskores::Id B>
int GreyPixel<B>::GetColorType()
{
  return internal::GreyColorType;
}

} // namespace io
} // namespace viskores

#endif //viskores_io_PixelTypes_h
