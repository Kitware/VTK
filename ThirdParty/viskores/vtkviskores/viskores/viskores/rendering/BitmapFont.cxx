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

#include <viskores/rendering/BitmapFont.h>

namespace viskores
{
namespace rendering
{

BitmapFont::BitmapFont()
{
  for (int i = 0; i < 256; ++i)
    ShortMap[i] = 0;
  this->PadL = 0;
  this->PadR = 0;
  this->PadT = 0;
  this->PadB = 0;
}

const viskores::rendering::BitmapFont::Character& BitmapFont::GetChar(char c) const
{
  std::size_t mappedCharIndex = static_cast<std::size_t>(this->ShortMap[(unsigned char)c]);
  return this->Chars[mappedCharIndex];
}

viskores::Float32 BitmapFont::GetTextWidth(const std::string& text) const
{
  viskores::Float32 width = 0;
  for (unsigned int i = 0; i < text.length(); ++i)
  {
    Character c = this->GetChar(text[i]);
    char nextchar = (i < text.length() - 1) ? text[i + 1] : 0;

    const bool kerning = true;
    if (kerning && nextchar > 0)
      width += viskores::Float32(c.kern[int(nextchar)]) / viskores::Float32(this->Height);
    width += viskores::Float32(c.adv) / viskores::Float32(this->Height);
  }
  return width;
}

void BitmapFont::GetCharPolygon(char character,
                                viskores::Float32& x,
                                viskores::Float32& y,
                                viskores::Float32& vl,
                                viskores::Float32& vr,
                                viskores::Float32& vt,
                                viskores::Float32& vb,
                                viskores::Float32& tl,
                                viskores::Float32& tr,
                                viskores::Float32& tt,
                                viskores::Float32& tb,
                                char nextchar) const
{
  Character c = this->GetChar(character);

  // By default, the origin for the font is at the
  // baseline.  That's nice, but we'd rather it
  // be at the actual bottom, so create an offset.
  viskores::Float32 yoff = -viskores::Float32(this->Descender) / viskores::Float32(this->Height);

  tl = viskores::Float32(c.x + this->PadL) / viskores::Float32(this->ImgW);
  tr = viskores::Float32(c.x + c.w - this->PadR) / viskores::Float32(this->ImgW);
  tt = 1.f - viskores::Float32(c.y + this->PadT) / viskores::Float32(this->ImgH);
  tb = 1.f - viskores::Float32(c.y + c.h - this->PadB) / viskores::Float32(this->ImgH);

  vl = x + viskores::Float32(c.offx + this->PadL) / viskores::Float32(this->Height);
  vr = x + viskores::Float32(c.offx + c.w - this->PadR) / viskores::Float32(this->Height);
  vt = yoff + y + viskores::Float32(c.offy - this->PadT) / viskores::Float32(this->Height);
  vb = yoff + y + viskores::Float32(c.offy - c.h + this->PadB) / viskores::Float32(this->Height);

  const bool kerning = true;
  if (kerning && nextchar > 0)
    x += viskores::Float32(c.kern[int(nextchar)]) / viskores::Float32(this->Height);
  x += viskores::Float32(c.adv) / viskores::Float32(this->Height);
}
}
} // namespace viskores::rendering
