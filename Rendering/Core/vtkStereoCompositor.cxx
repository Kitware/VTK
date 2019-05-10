/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStereoCompositor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStereoCompositor.h"

#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <cassert>

vtkStandardNewMacro(vtkStereoCompositor);
//----------------------------------------------------------------------------
vtkStereoCompositor::vtkStereoCompositor() {}

//----------------------------------------------------------------------------
vtkStereoCompositor::~vtkStereoCompositor() {}

//----------------------------------------------------------------------------
bool vtkStereoCompositor::Validate(vtkUnsignedCharArray* rgbLeftNResult,
  vtkUnsignedCharArray* rgbRight, const int* size /*=nullptr*/)
{
  assert(rgbLeftNResult != nullptr || rgbRight != nullptr);
  if (rgbLeftNResult->GetNumberOfComponents() != 3 || rgbRight->GetNumberOfComponents() != 3)
  {
    vtkErrorMacro("vtkStereoCompositor only support 3 component arrays.");
    return false;
  }

  if (rgbLeftNResult->GetNumberOfTuples() != rgbRight->GetNumberOfTuples())
  {
    vtkErrorMacro("Mismatch in number of tuples between left and right eye images.");
    return false;
  }

  if (size != nullptr &&
    rgbLeftNResult->GetNumberOfTuples() != static_cast<vtkIdType>(size[0] * size[1]))
  {
    vtkErrorMacro("Mismatch number of tuples and image size.");
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkStereoCompositor::RedBlue(
  vtkUnsignedCharArray* rgbLeftNResult, vtkUnsignedCharArray* rgbRight)
{
  if (!this->Validate(rgbLeftNResult, rgbRight))
  {
    return false;
  }

  vtkSMPTools::For(0, rgbLeftNResult->GetNumberOfTuples(),
    [rgbLeftNResult, rgbRight](vtkIdType first, vtkIdType last) {
      unsigned char value[3];
      unsigned char output[3] = { 0, 0, 0 };
      for (auto cc = first; cc < last; ++cc)
      {
        rgbLeftNResult->GetTypedTuple(cc, value);
        output[0] = (value[0] + value[1] + value[2]) / 3;
        // output[1] = 0; (fyi)
        rgbRight->GetTypedTuple(cc, value);
        output[2] = (value[0] + value[1] + value[2]) / 3;

        rgbLeftNResult->SetTypedTuple(cc, output);
      }
    });
  return true;
}

//----------------------------------------------------------------------------
bool vtkStereoCompositor::Anaglyph(vtkUnsignedCharArray* rgbLeftNResult,
  vtkUnsignedCharArray* rgbRight, float colorSaturation, const int colorMask[2])
{
  if (!this->Validate(rgbLeftNResult, rgbRight))
  {
    return false;
  }

  // build some tables
  auto a = colorSaturation;
  auto m0 = colorMask[0];
  auto m1 = colorMask[1];

  int avecolor[256][3], satcolor[256];
  for (int x = 0; x < 256; x++)
  {
    avecolor[x][0] = int((1.0 - a) * x * 0.3086);
    avecolor[x][1] = int((1.0 - a) * x * 0.6094);
    avecolor[x][2] = int((1.0 - a) * x * 0.0820);

    satcolor[x] = int(a * x);
  }

  vtkSMPTools::For(0, rgbLeftNResult->GetNumberOfTuples(), [&](vtkIdType first, vtkIdType last) {
    unsigned char inL[3], inR[3];
    unsigned char out[3];
    for (auto cc = first; cc < last; ++cc)
    {
      rgbLeftNResult->GetTypedTuple(cc, inL);
      rgbRight->GetTypedTuple(cc, inR);

      auto ave0 = avecolor[inL[0]][0] + avecolor[inL[1]][1] + avecolor[inL[2]][2];
      auto ave1 = avecolor[inR[0]][0] + avecolor[inR[1]][1] + avecolor[inR[2]][2];

      if (m0 & 0x4)
      {
        out[0] = satcolor[inL[0]] + ave0;
      }
      if (m0 & 0x2)
      {
        out[1] = satcolor[inL[1]] + ave0;
      }
      if (m0 & 0x1)
      {
        out[2] = satcolor[inL[2]] + ave0;
      }
      if (m1 & 0x4)
      {
        out[0] = satcolor[inR[0]] + ave1;
      }
      if (m1 & 0x2)
      {
        out[1] = satcolor[inR[1]] + ave1;
      }
      if (m1 & 0x1)
      {
        out[2] = satcolor[inR[2]] + ave1;
      }
      rgbLeftNResult->SetTypedTuple(cc, out);
    }
  });

  return true;
}

//----------------------------------------------------------------------------
bool vtkStereoCompositor::Interlaced(
  vtkUnsignedCharArray* rgbLeftNResult, vtkUnsignedCharArray* rgbRight, const int size[2])
{
  if (!this->Validate(rgbLeftNResult, rgbRight, size))
  {
    return false;
  }

  const auto line = size[0] * 3;
  auto rptr = rgbRight->GetPointer(0);
  auto optr = rgbLeftNResult->GetPointer(0);
  rptr += line;
  optr += line;
  for (int y = 1; y < size[1]; y += 2)
  {
    std::copy(rptr, rptr + line, optr);
    // advance copied line.
    rptr += line;
    optr += line;

    // skip a line.
    rptr += line;
    optr += line;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkStereoCompositor::Dresden(
  vtkUnsignedCharArray* rgbLeftNResult, vtkUnsignedCharArray* rgbRight, const int size[2])
{
  if (!this->Validate(rgbLeftNResult, rgbRight, size))
  {
    return false;
  }

  auto rptr = rgbRight->GetPointer(0);
  auto optr = rgbLeftNResult->GetPointer(0);
  optr += 3;
  rptr += 3;
  for (int y = 0; y < size[1]; y++)
  {
    for (int x = 1; x < size[0]; x += 2)
    {
      *optr++ = *rptr++;
      *optr++ = *rptr++;
      *optr++ = *rptr++;

      optr += 3;
      rptr += 3;
    }
    if (size[0] % 2 == 1)
    {
      optr += 3;
      rptr += 3;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkStereoCompositor::Checkerboard(
  vtkUnsignedCharArray* rgbLeftNResult, vtkUnsignedCharArray* rgbRight, const int size[2])
{
  if (!this->Validate(rgbLeftNResult, rgbRight, size))
  {
    return false;
  }

  for (int y = 0; y < size[1]; ++y)
  {
    // set up the pointers
    // right starts on x = 1 on even scanlines
    // right starts on x = 0 on odd scanlines
    auto offset = y % 2 == 0 ? y * 3 * size[0] + 3 : y * 3 * size[0];
    auto rptr = rgbRight->GetPointer(offset);
    auto optr = rgbLeftNResult->GetPointer(offset);

    // skip every other pixel.
    for (int x = (y + 1) % 2; x < size[0]; x += 2)
    {
      *optr++ = *rptr++;
      *optr++ = *rptr++;
      *optr++ = *rptr++;

      // skip pixel.
      optr += 3;
      rptr += 3;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkStereoCompositor::SplitViewportHorizontal(
  vtkUnsignedCharArray* rgbLeftNResult, vtkUnsignedCharArray* rgbRight, const int size[2])
{
  if (!this->Validate(rgbLeftNResult, rgbRight, size))
  {
    return false;
  }

  auto sleft = rgbLeftNResult->GetPointer(0);
  auto sright = rgbRight->GetPointer(0);

  int midX = static_cast<int>(size[0] / 2.0);

  // If the row size is even, reduce the row copy by
  // one. Otherwise the pointer will overflow when we fill the
  // right hand part of the stereo.
  if (size[0] % 2 == 0)
  {
    midX--;
  }

  const int offsetX = static_cast<int>(ceil(size[0] / 2.0));

  // copy pixel data
  for (int y = 0; y <= (size[1] - 1); ++y)
  {
    for (int x = 1; x <= midX; ++x)
    {
      auto left = sleft + (x * 3) + (y * size[0] * 3);
      auto leftTemp = sleft + ((2 * x) * 3) + (y * size[0] * 3);
      *left++ = *leftTemp++;
      *left++ = *leftTemp++;
      *left++ = *leftTemp++;
    }
  }

  for (int y = 0; y <= (size[1] - 1); ++y)
  {
    for (int x = 0; x < midX; ++x)
    {
      auto left = sleft + ((x + offsetX) * 3) + (y * size[0] * 3);
      auto right = sright + ((2 * x) * 3) + (y * size[0] * 3);
      *left++ = *right++;
      *left++ = *right++;
      *left++ = *right++;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkStereoCompositor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
