/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorSeries.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkColorSeries.h"

#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"

#include <vector>
#include <sstream>

//-----------------------------------------------------------------------------
class vtkColorSeriesPalette
{
public:
  std::vector<vtkColor3ub> Colors;
  vtkStdString Name;
};

//-----------------------------------------------------------------------------
class vtkColorSeries::Private
{
public:
  Private();

  void SetScheme(int idx);
  int SetSchemeByName(const vtkStdString& name, bool& modified);

  std::vector<vtkColorSeriesPalette> Palettes; // All palettes
  int Palette; // Currently-selected entry in Palettes
  std::vector<vtkColor3ub>* Colors; // Pointer to colors in current scheme
};

//-----------------------------------------------------------------------------
inline vtkColor3ub vtkColor3ubFromHex3(vtkTypeUInt32 hex)
{
  int b = hex & 0xff; hex >>= 8;
  int g = hex & 0xff; hex >>= 8;
  int r = hex & 0xff;
  return vtkColor3ub(r, g, b);
}

//-----------------------------------------------------------------------------
vtkColorSeries::Private::Private()
{
  this->Palettes.resize(vtkColorSeries::CUSTOM);
  vtkTypeUInt32 colors[] =
    {
    // Original vtkColorSeries palettes, not part of the Brewer schemes
    vtkColorSeries::SPECTRUM,
    0x000000, 0xE41A1C, 0x377EB8, 0x4DAF4A, 0x984EA3, 0xFF7F00, 0xA65628,
    vtkColorSeries::WARM,
    0x791717, 0xB50101, 0xEF4719, 0xF98324, 0xFFB400, 0xFFE506,
    vtkColorSeries::COOL,
    0x75B101, 0x588029, 0x50D7BF, 0x1C95CD, 0x3B68AB, 0x9A68FF, 0x5F3380,
    vtkColorSeries::BLUES,
    0x3B68AB, 0x1C95CD, 0x4ED9EA, 0x739AD5, 0x423DA9, 0x505487, 0x102A52,
    vtkColorSeries::WILD_FLOWER,
    0x1C95CD, 0x3B68AB, 0x663EB7, 0xA254CF, 0xDE61CE, 0xDC6195, 0x3D1052,
    vtkColorSeries::CITRUS,
    0x657C37, 0x75B101, 0xB2BA30, 0xFFE506, 0xFFB400, 0xF98324,

/*
The following palettes are colors from www.ColorBrewer2.org by
Cynthia A. Brewer, Geography, Pennsylvania State University.
Use the sentence above or the following bibliography entry to credit her:

+ Brewer, Cynthia A. and Mark Harrower and Andy Woodruff and David Heyman,
  2010. http://ColorBrewer2.org, accessed 2010-Nov-9.

The color schemes below are copyright under the following license, excerpted
from http://www.personal.psu.edu/cab38/ColorBrewer/ColorBrewer_updates.html
on August 13, 2012:

    Apache-Style Software License for ColorBrewer software and
    ColorBrewer Color Schemes

    Copyright (c) 2002 Cynthia Brewer, Mark Harrower, and The Pennsylvania
    State University.

    Licensed under the Apache License, Version 2.0 (the "License"); you may not
    use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
    License for the specific language governing permissions and limitations
    under the License.

This text from my earlier Apache License Version 1.1 also remains in place for
guidance on attribution and permissions:

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions as source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
    2. The end-user documentation included with the redistribution, if any, must
       include the following acknowledgment:
       "This product includes color specifications and designs developed by
       Cynthia Brewer (http://colorbrewer.org/)."
       Alternately, this acknowledgment may appear in the software itself, if
       and wherever such third-party acknowledgments normally appear.
    4. The name "ColorBrewer" must not be used to endorse or promote products
       derived from this software without prior written permission. For written
       permission, please contact Cynthia Brewer at cbrewer@psu.edu.
    5. Products derived from this software may not be called "ColorBrewer", nor
       may "ColorBrewer" appear in their name, without prior written permission
       of Cynthia Brewer.
*/
    // Diverging
    //   Purple-Orange
    vtkColorSeries::BREWER_DIVERGING_PURPLE_ORANGE_11,
    0x7F3B08, 0xB35806, 0xE08214, 0xFDB863, 0xFEE0B6, 0xF7F7F7, 0xD8DAEB, 0xB2ABD2, 0x8073AC, 0x542788, 0x2D004B,
    vtkColorSeries::BREWER_DIVERGING_PURPLE_ORANGE_10,
    0x7F3B08, 0xB35806, 0xE08214, 0xFDB863, 0xFEE0B6, 0xD8DAEB, 0xB2ABD2, 0x8073AC, 0x542788, 0x2D004B,
    vtkColorSeries::BREWER_DIVERGING_PURPLE_ORANGE_9,
    0xB35806, 0xE08214, 0xFDB863, 0xFEE0B6, 0xF7F7F7, 0xD8DAEB, 0xB2ABD2, 0x8073AC, 0x542788,
    vtkColorSeries::BREWER_DIVERGING_PURPLE_ORANGE_8,
    0xB35806, 0xE08214, 0xFDB863, 0xFEE0B6, 0xD8DAEB, 0xB2ABD2, 0x8073AC, 0x542788,
    vtkColorSeries::BREWER_DIVERGING_PURPLE_ORANGE_7,
    0xB35806, 0xF1A340, 0xFEE0B6, 0xF7F7F7, 0xD8DAEB, 0x998EC3, 0x542788,
    vtkColorSeries::BREWER_DIVERGING_PURPLE_ORANGE_6,
    0xB35806, 0xF1A340, 0xFEE0B6, 0xD8DAEB, 0x998EC3, 0x542788,
    vtkColorSeries::BREWER_DIVERGING_PURPLE_ORANGE_5,
    0xE66101, 0xFDB863, 0xF7F7F7, 0xB2ABD2, 0x5E3C99,
    vtkColorSeries::BREWER_DIVERGING_PURPLE_ORANGE_4,
    0xE66101, 0xFDB863, 0xB2ABD2, 0x5E3C99,
    vtkColorSeries::BREWER_DIVERGING_PURPLE_ORANGE_3,
    0xF1A340, 0xF7F7F7, 0x998EC3,
    //   Spectral
    vtkColorSeries::BREWER_DIVERGING_SPECTRAL_11,
    0x9E0142, 0xD53E4F, 0xF46D43, 0xFDAE61, 0xFEE08B, 0xFFFFBF, 0xE6F598, 0xABDDA4, 0x66C2A5, 0x3288BD, 0x5E4FA2,
    vtkColorSeries::BREWER_DIVERGING_SPECTRAL_10,
    0x9E0142, 0xD53E4F, 0xF46D43, 0xFDAE61, 0xFEE08B, 0xE6F598, 0xABDDA4, 0x66C2A5, 0x3288BD, 0x5E4FA2,
    vtkColorSeries::BREWER_DIVERGING_SPECTRAL_9,
    0xD53E4F, 0xF46D43, 0xFDAE61, 0xFEE08B, 0xFFFFBF, 0xE6F598, 0xABDDA4, 0x66C2A5, 0x3288BD,
    vtkColorSeries::BREWER_DIVERGING_SPECTRAL_8,
    0xD53E4F, 0xF46D43, 0xFDAE61, 0xFEE08B, 0xE6F598, 0xABDDA4, 0x66C2A5, 0x3288BD,
    vtkColorSeries::BREWER_DIVERGING_SPECTRAL_7,
    0xD53E4F, 0xFC8D59, 0xFEE08B, 0xFFFFBF, 0xE6F598, 0x99D594, 0x3288BD,
    vtkColorSeries::BREWER_DIVERGING_SPECTRAL_6,
    0xD53E4F, 0xFC8D59, 0xFEE08B, 0xE6F598, 0x99D594, 0x3288BD,
    vtkColorSeries::BREWER_DIVERGING_SPECTRAL_5,
    0xD7191C, 0xFDAE61, 0xFFFFBF, 0xABDDA4, 0x2B83BA,
    vtkColorSeries::BREWER_DIVERGING_SPECTRAL_4,
    0xD7191C, 0xFDAE61, 0xABDDA4, 0x2B83BA,
    vtkColorSeries::BREWER_DIVERGING_SPECTRAL_3,
    0xFC8D59, 0xFFFFBF, 0x99D594,
    //   Brown-Blue-Green
    vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_11,
    0x543005, 0x8C510A, 0xBF812D, 0xDFC27D, 0xF6E8C3, 0xF5F5F5, 0xC7EAE5, 0x80CDC1, 0x35978F, 0x01665E, 0x003C30,
    vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_10,
    0x543005, 0x8C510A, 0xBF812D, 0xDFC27D, 0xF6E8C3, 0xC7EAE5, 0x80CDC1, 0x35978F, 0x01665E, 0x003C30,
    vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_9,
    0x8C510A, 0xBF812D, 0xDFC27D, 0xF6E8C3, 0xF5F5F5, 0xC7EAE5, 0x80CDC1, 0x35978F, 0x01665E,
    vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_8,
    0x8C510A, 0xBF812D, 0xDFC27D, 0xF6E8C3, 0xC7EAE5, 0x80CDC1, 0x35978F, 0x01665E,
    vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_7,
    0x8C510A, 0xD8B365, 0xF6E8C3, 0xF5F5F5, 0xC7EAE5, 0x5AB4AC, 0x01665E,
    vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_6,
    0x8C510A, 0xD8B365, 0xF6E8C3, 0xC7EAE5, 0x5AB4AC, 0x01665E,
    vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_5,
    0xA6611A, 0xDFC27D, 0xF5F5F5, 0x80CDC1, 0x018571,
    vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_4,
    0xA6611A, 0xDFC27D, 0x80CDC1, 0x018571,
    vtkColorSeries::BREWER_DIVERGING_BROWN_BLUE_GREEN_3,
    0xD8B365, 0xF5F5F5, 0x5AB4AC,
    // Sequential Palettes
    //   Blue-Green
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_GREEN_9,
    0xF7FCFD, 0xE5F5F9, 0xCCECE6, 0x99D8C9, 0x66C2A4, 0x41AE76, 0x238B45, 0x006D2C, 0x00441B,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_GREEN_8,
    0xF7FCFD, 0xE5F5F9, 0xCCECE6, 0x99D8C9, 0x66C2A4, 0x41AE76, 0x238B45, 0x005824,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_GREEN_7,
    0xEDF8FB, 0xCCECE6, 0xCCECE6, 0x66C2A4, 0x41AE76, 0x238B45, 0x005824,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_GREEN_6,
    0xEDF8FB, 0xCCECE6, 0x99D8C9, 0x66C2A4, 0x2CA25F, 0x006D2C,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_GREEN_5,
    0xEDF8FB, 0xB2E2E2, 0x66C2A4, 0x2CA25F, 0x006D2C,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_GREEN_4,
    0xEDF8FB, 0xB2E2E2, 0x66C2A4, 0x238B45,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_GREEN_3,
    0xE5F5F9, 0x99D8C9, 0x2CA25F,
    //   Yellow-Orange-Brown
    vtkColorSeries::BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_9,
    0xFFFFE5, 0xFFF7BC, 0xFEE391, 0xFEC44F, 0xFE9929, 0xEC7014, 0xCC4C02, 0x993404, 0x662506,
    vtkColorSeries::BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_8,
    0xFFFFE5, 0xFFF7BC, 0xFEE391, 0xFEC44F, 0xFE9929, 0xEC7014, 0xCC4C02, 0x8C2D04,
    vtkColorSeries::BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_7,
    0xFFFFD4, 0xFEE391, 0xFEC44F, 0xFE9929, 0xEC7014, 0xCC4C02, 0x8C2D04,
    vtkColorSeries::BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_6,
    0xFFFFD4, 0xFEE391, 0xFEC44F, 0xFE9929, 0xD95F0E, 0x993404,
    vtkColorSeries::BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_5,
    0xFFFFD4, 0xFED98E, 0xFE9929, 0xD95F0E, 0x993404,
    vtkColorSeries::BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_4,
    0xFFFFD4, 0xFED98E, 0xFE9929, 0xCC4C02,
    vtkColorSeries::BREWER_SEQUENTIAL_YELLOW_ORANGE_BROWN_3,
    0xFFF7BC, 0xFEC44F, 0xD95F0E,
    //   Blue-Purple
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_PURPLE_9,
    0xF7FCFD, 0xE0ECF4, 0xBFD3E6, 0x9EBCDA, 0x8C96C6, 0x8C6BB1, 0x88419D, 0x810F7C, 0x4D004B,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_PURPLE_8,
    0xF7FCFD, 0xE0ECF4, 0xBFD3E6, 0x9EBCDA, 0x8C96C6, 0x8C6BB1, 0x88419D, 0x6E016B,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_PURPLE_7,
    0xEDF8FB, 0xBFD3E6, 0x9EBCDA, 0x8C96C6, 0x8C6BB1, 0x88419D, 0x6E016B,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_PURPLE_6,
    0xEDF8FB, 0xBFD3E6, 0x9EBCDA, 0x8C96C6, 0x8856A7, 0x810F7C,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_PURPLE_5,
    0xEDF8FB, 0xB3CDE3, 0x8C96C6, 0x8856A7, 0x810F7C,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_PURPLE_4,
    0xEDF8FB, 0xB3CDE3, 0x8C96C6, 0x88419D,
    vtkColorSeries::BREWER_SEQUENTIAL_BLUE_PURPLE_3,
    0xE0ECF4, 0x9EBCDA, 0x8856A7,
    // Qualitative Palettes
    //   Accent
    vtkColorSeries::BREWER_QUALITATIVE_ACCENT,
    0x7FC97F, 0xBEAED4, 0xFDC086, 0xFFFF99, 0x386CB0, 0xF0027F, 0xBF5B17, 0x666666,
    //   Dark2
    vtkColorSeries::BREWER_QUALITATIVE_DARK2,
    0x1B9E77, 0xD95F02, 0x7570B3, 0xE7298A, 0x66A61E, 0xE6AB02, 0xA6761D, 0x666666,
    //   Set2
    vtkColorSeries::BREWER_QUALITATIVE_SET2,
    0x66C2A5, 0xFC8D62, 0x8DA0CB, 0xE78AC3, 0xA6D854, 0xFFD92F, 0xE5C494, 0xB3B3B3,
    //   Pastel2
    vtkColorSeries::BREWER_QUALITATIVE_PASTEL2,
    0xB3E2CD, 0xFDCDAC, 0xCBD5E8, 0xF4CAE4, 0xE6F5C9, 0xFFF2AE, 0xF1E2CC, 0xCCCCCC,
    //   Pastel1
    vtkColorSeries::BREWER_QUALITATIVE_PASTEL1,
    0xFBB4AE, 0xB3CDE3, 0xCCEBC5, 0xDECBE4, 0xFED9A6, 0xFFFFCC, 0xE5D8BD, 0xFDDAEC, 0xF2F2F2,
    //   Set1
    vtkColorSeries::BREWER_QUALITATIVE_SET1,
    0xE41A1C, 0x377EB8, 0x4DAF4A, 0x984EA3, 0xFF7F00, 0xFFFF33, 0xA65628, 0xF781BF, 0x999999,
    //   Paired
    vtkColorSeries::BREWER_QUALITATIVE_PAIRED,
    0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99,
    //   Set3
    vtkColorSeries::BREWER_QUALITATIVE_SET3,
    0x8DD3C7, 0xFFFFB3, 0xBEBADA, 0xFB8072, 0x80B1D3, 0xFDB462, 0xB3DE69, 0xFCCDE5, 0xD9D9D9, 0xBC80BD, 0xCCEBC5, 0xFFED6F,
    };
  const char* names[] =
    {
    "Spectrum",
    "Warm",
    "Cool",
    "Blues",
    "Wild Flower",
    "Citrus",

    "Brewer Diverging Purple-Orange",
    "Brewer Diverging Spectral",
    "Brewer Diverging Brown-Blue-Green",

    "Brewer Sequential Blue-Green",
    "Brewer Sequential Yellow-Orange-Brown",
    "Brewer Sequential Blue-Purple",

    "Brewer Qualitative Accent",
    "Brewer Qualitative Dark2",
    "Brewer Qualitative Set2",
    "Brewer Qualitative Pastel2",
    "Brewer Qualitative Pastel1",
    "Brewer Qualitative Set1",
    "Brewer Qualitative Paired",
    "Brewer Qualitative Set3"
    };
  int sizes[][2] =
    {
      {  7,  7 },
      {  6,  6 },
      {  7,  7 },
      {  7,  7 },
      {  7,  7 },
      {  6,  6 },

      { 11,  3 },
      { 11,  3 },
      { 11,  3 },
      {  9,  3 },
      {  9,  3 },
      {  9,  3 },
      {  8,  8 },
      {  8,  8 },
      {  8,  8 },
      {  8,  8 },
      {  9,  9 },
      {  9,  9 },
      { 11, 11 },
      { 12, 12 }
    };
  vtkTypeUInt32* color = colors;
  vtkColorSeriesPalette* pal;
  for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
  {
    int start = sizes[i][0];
    int stop  = sizes[i][1];
    int step = start > stop ? -1 : 1;
    for (int n = start; n != stop + step; n += step)
    {
      int paletteIndex = *(color++);
      pal = &(this->Palettes[paletteIndex]);
      std::ostringstream os;
      os << names[i];
      if (start != stop)
      {
        os << " (" << n << ")";
      }
      pal->Name = os.str();
      for (int j = 0; j < n; ++j)
      {
        pal->Colors.push_back(vtkColor3ubFromHex3(*(color++)));
      }
    }
  }
  this->Colors = 0;
  this->Palette = vtkColorSeries::SPECTRUM;
  this->Colors = &(this->Palettes[this->Palette].Colors);
}

//-----------------------------------------------------------------------------
void vtkColorSeries::Private::SetScheme(int idx)
{
  this->Colors = &(this->Palettes[idx].Colors);
  this->Palette = idx;
}

//-----------------------------------------------------------------------------
int vtkColorSeries::Private::SetSchemeByName(
  const vtkStdString& name, bool& modified)
{
  modified = false;
  int idx = 0;
  std::vector<vtkColorSeriesPalette>::iterator it;
  for (it = this->Palettes.begin(); it != this->Palettes.end(); ++it, ++idx)
  {
    if (it->Name == name)
    {
      this->SetScheme(idx);
      return idx;
    }
  }
  // OK, we could not find such a palette. Create one.
  modified = true;
  vtkColorSeriesPalette blank;
  blank.Name = name;
  idx = static_cast<int>(this->Palettes.size());
  this->Palettes.push_back(blank);
  this->SetScheme(idx);
  return idx;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkColorSeries);

//-----------------------------------------------------------------------------
vtkColorSeries::vtkColorSeries()
{
  this->Storage = new vtkColorSeries::Private;
  this->SetColorScheme(vtkColorSeries::SPECTRUM);
}

//-----------------------------------------------------------------------------
vtkColorSeries::~vtkColorSeries()
{
  delete this->Storage;
  this->Storage = NULL;
}

//-----------------------------------------------------------------------------
void vtkColorSeries::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  int pidx = this->Storage->Palette;
  vtkColorSeriesPalette* palette = &(this->Storage->Palettes[pidx]);
  os
    << indent << "ColorScheme: " << pidx << endl
    << indent << "ColorSchemeName : "
    << (palette->Name.empty() ? "(empty)" : palette->Name.c_str()) << endl;
}

//-----------------------------------------------------------------------------
void vtkColorSeries::SetColorScheme(int scheme)
{
  if (this->Storage->Palette == scheme)
  {
    return;
  }

  if (scheme < 0 || scheme >= this->GetNumberOfColorSchemes())
  {
    vtkWarningMacro(<< "Scheme " << scheme << " out of range. Ignoring.");
    return;
  }

  this->Storage->SetScheme(scheme);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkColorSeries::SetColorSchemeByName(const vtkStdString& schemeName)
{
  bool modified;
  int index = this->Storage->SetSchemeByName(schemeName, modified);
  if (modified)
  {
    this->Modified();
  }
  return index;
}

//-----------------------------------------------------------------------------
int vtkColorSeries::GetNumberOfColorSchemes() const
{
  return static_cast<int>(this->Storage->Palettes.size());
}

//-----------------------------------------------------------------------------
vtkStdString vtkColorSeries::GetColorSchemeName() const
{
  return this->Storage->Palettes[this->Storage->Palette].Name;
}

//-----------------------------------------------------------------------------
void vtkColorSeries::SetColorSchemeName(const vtkStdString& name)
{
  // Ignore empty names
  // TODO: Should we prohibit duplicate names? If not,
  // how about searching backwards through palettes in
  // SetColorSchemeByName() so users can override
  // system defaults?
  if ( name.empty() )
  {
    return;
  }

  this->CopyOnWrite();
  if (this->Storage->Palettes[this->Storage->Palette].Name != name)
  {
    this->Storage->Palettes[this->Storage->Palette].Name = name;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int vtkColorSeries::GetColorScheme() const
{
  return this->Storage->Palette;
}

//-----------------------------------------------------------------------------
int vtkColorSeries::GetNumberOfColors() const
{
  return static_cast<int>(this->Storage->Colors->size());
}

//-----------------------------------------------------------------------------
void vtkColorSeries::SetNumberOfColors(int numColors)
{
  this->CopyOnWrite();
  this->Storage->Colors->resize(numColors);
}

//-----------------------------------------------------------------------------
vtkColor3ub vtkColorSeries::GetColor(int index) const
{
  if (index >=0 && index < static_cast<int>(this->Storage->Colors->size()))
  {
    return (*this->Storage->Colors)[index];
  }
  else
  {
    return vtkColor3ub(0,0,0);
  }
}

//-----------------------------------------------------------------------------
vtkColor3ub vtkColorSeries::GetColorRepeating(int index) const
{
  vtkColor3ub color;
  int numColors = this->GetNumberOfColors();
  // If we have an empty palette, index % numColors generates a divide-by-zero
  // fault, and if it did return a valid value, looking up the resulting color
  // would be an access violation. So, be careful here:
  color = numColors ?
    (*this->Storage->Colors)[index % numColors] :
    vtkColor3ub(0,0,0);
  return color;
}

//-----------------------------------------------------------------------------
void vtkColorSeries::SetColor(int index, const vtkColor3ub &color)
{
  if (index >=0 && index < static_cast<int>(this->Storage->Colors->size()))
  {
    this->CopyOnWrite();
    (*this->Storage->Colors)[index] = color;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkColorSeries::AddColor(const vtkColor3ub &color)
{
  this->CopyOnWrite();
  this->Storage->Colors->push_back(color);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkColorSeries::InsertColor(int index, const vtkColor3ub &color)
{
  if (index >=0 && index < static_cast<int>(this->Storage->Colors->size()))
  {
    this->CopyOnWrite();
    this->Storage->Colors->insert(
      this->Storage->Colors->begin() + index, color);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkColorSeries::RemoveColor(int index)
{
  if (index >=0 && index < static_cast<int>(this->Storage->Colors->size()))
  {
    this->CopyOnWrite();
    this->Storage->Colors->erase(this->Storage->Colors->begin() + index);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkColorSeries::ClearColors()
{
  this->CopyOnWrite();
  this->Storage->Colors->clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkColorSeries::DeepCopy(vtkColorSeries* colors)
{
  if (! colors)
  {
    return;
  }

  this->Storage->Palettes = colors->Storage->Palettes;
  this->Storage->Palette = colors->Storage->Palette;
  this->Storage->Colors =
    &(this->Storage->Palettes[this->Storage->Palette].Colors);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkColorSeries::BuildLookupTable(vtkLookupTable* lkup, int lutIndexing)
{
  if (lkup)
  {
    lkup->SetNumberOfTableValues(this->GetNumberOfColors());
    lkup->SetIndexedLookup((lutIndexing == ORDINAL) ? 0 : 1);
    for (int i = 0; i < this->GetNumberOfColors(); ++i)
    {
      vtkColor3ub colr = this->GetColor(i);
      lkup->SetTableValue(
        i, colr.GetRed()/255., colr.GetGreen()/255., colr.GetBlue()/255., 1.);
    }
  }
}

//-----------------------------------------------------------------------------
vtkLookupTable* vtkColorSeries::CreateLookupTable(int lutIndexing)
{
  vtkLookupTable *lkup = vtkLookupTable::New();
  this->BuildLookupTable(lkup, lutIndexing);
  return lkup;
}

//-----------------------------------------------------------------------------
void vtkColorSeries::CopyOnWrite()
{
  // If the current scheme is predefined, copy it to a new, custom scheme.
  int prevScheme = this->Storage->Palette;
  if (prevScheme < CUSTOM)
  {
    int nextScheme = static_cast<int>(this->Storage->Palettes.size());
    vtkColorSeriesPalette blank;
    blank.Name = this->Storage->Palettes[prevScheme].Name + " copy";
    this->Storage->Palettes.push_back(blank);
    this->Storage->SetScheme(nextScheme);
    *this->Storage->Colors = this->Storage->Palettes[prevScheme].Colors;
    this->Modified();
  }
}
