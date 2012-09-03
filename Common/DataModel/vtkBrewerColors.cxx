#include "vtkBrewerColors.h"

#include "vtkColor.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkLookupTable.h"

#include <map>
#include <vector>

#include <vtksys/ios/sstream>

class vtkColorSchemeInternal : public std::map<vtkStdString,std::vector<vtkColor4ub> >
{
};

inline vtkColor4ub vtkColor4ubFromHex3( vtkTypeUInt32 hex )
{
  int b = hex & 0xff; hex >>= 8;
  int g = hex & 0xff; hex >>= 8;
  int r = hex & 0xff;
  return vtkColor4ub( r, g, b, 0xff );
}

static void constructDefaultSchemes( vtkBrewerColors* schemes )
{
/*
The following palettes are colors from www.ColorBrewer2.org by Cynthia A. Brewer, Geography, Pennsylvania State University.
Use the sentence above or the following bibliography entry to credit her:

+ Brewer, Cynthia A. and Mark Harrower and Andy Woodruff and David Heyman, 2010. http://ColorBrewer2.org, accessed 2010-Nov-9.

The color schemes below are copyright under the following license, excerpted
from http://www.personal.psu.edu/cab38/ColorBrewer/ColorBrewer_updates.html
on August 13, 2012:

    Apache-Style Software License for ColorBrewer software and ColorBrewer Color Schemes

    Copyright (c) 2002 Cynthia Brewer, Mark Harrower, and The Pennsylvania State University.

    Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software distributed
    under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied. See the License for the
    specific language governing permissions and limitations under the License.

    This text from my earlier Apache License Version 1.1 also remains in place for guidance on attribution and permissions:

    Redistribution and use in source and binary forms, with or without modification, are permitted
    provided that the following conditions are met:
    1. Redistributions as source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    2. The end-user documentation included with the redistribution, if any, must include the following acknowledgment:
    "This product includes color specifications and designs developed by Cynthia Brewer (http://colorbrewer.org/)."
    Alternately, this acknowledgment may appear in the software itself, if and wherever such third-party acknowledgments
    normally appear.
    4. The name "ColorBrewer" must not be used to endorse or promote products derived from this software without
    prior written permission. For written permission, please contact Cynthia Brewer at cbrewer@psu.edu.
    5. Products derived from this software may not be called "ColorBrewer", nor may "ColorBrewer" appear in their name,
    without prior written permission of Cynthia Brewer.
*/
  vtkTypeUInt32 colors[] =
    {
    // Diverging
    //   Purple-Orange
    0x7F3B08, 0xB35806, 0xE08214, 0xFDB863, 0xFEE0B6, 0xF7F7F7, 0xD8DAEB, 0xB2ABD2, 0x8073AC, 0x542788, 0x2D004B,
    0x7F3B08, 0xB35806, 0xE08214, 0xFDB863, 0xFEE0B6, 0xD8DAEB, 0xB2ABD2, 0x8073AC, 0x542788, 0x2D004B,
    0xB35806, 0xE08214, 0xFDB863, 0xFEE0B6, 0xF7F7F7, 0xD8DAEB, 0xB2ABD2, 0x8073AC, 0x542788,
    0xB35806, 0xE08214, 0xFDB863, 0xFEE0B6, 0xD8DAEB, 0xB2ABD2, 0x8073AC, 0x542788,
    0xB35806, 0xF1A340, 0xFEE0B6, 0xF7F7F7, 0xD8DAEB, 0x998EC3, 0x542788,
    0xB35806, 0xF1A340, 0xFEE0B6, 0xD8DAEB, 0x998EC3, 0x542788,
    0xE66101, 0xFDB863, 0xF7F7F7, 0xB2ABD2, 0x5E3C99,
    0xE66101, 0xFDB863, 0xB2ABD2, 0x5E3C99,
    0xF1A340, 0xF7F7F7, 0x998EC3,
    //   Spectral
    0x9E0142, 0xD53E4F, 0xF46D43, 0xFDAE61, 0xFEE08B, 0xFFFFBF, 0xE6F598, 0xABDDA4, 0x66C2A5, 0x3288BD, 0x5E4FA2,
    0x9E0142, 0xD53E4F, 0xF46D43, 0xFDAE61, 0xFEE08B, 0xE6F598, 0xABDDA4, 0x66C2A5, 0x3288BD, 0x5E4FA2,
    0xD53E4F, 0xF46D43, 0xFDAE61, 0xFEE08B, 0xFFFFBF, 0xE6F598, 0xABDDA4, 0x66C2A5, 0x3288BD,
    0xD53E4F, 0xF46D43, 0xFDAE61, 0xFEE08B, 0xE6F598, 0xABDDA4, 0x66C2A5, 0x3288BD,
    0xD53E4F, 0xFC8D59, 0xFEE08B, 0xFFFFBF, 0xE6F598, 0x99D594, 0x3288BD,
    0xD53E4F, 0xFC8D59, 0xFEE08B, 0xE6F598, 0x99D594, 0x3288BD,
    0xD7191C, 0xFDAE61, 0xFFFFBF, 0xABDDA4, 0x2B83BA,
    0xD7191C, 0xFDAE61, 0xABDDA4, 0x2B83BA,
    0xFC8D59, 0xFFFFBF, 0x99D594,
    //   Brown-Blue-Green
    0x543005, 0x8C510A, 0xBF812D, 0xDFC27D, 0xF6E8C3, 0xF5F5F5, 0xC7EAE5, 0x80CDC1, 0x35978F, 0x01665E, 0x003C30,
    0x543005, 0x8C510A, 0xBF812D, 0xDFC27D, 0xF6E8C3, 0xC7EAE5, 0x80CDC1, 0x35978F, 0x01665E, 0x003C30,
    0x8C510A, 0xBF812D, 0xDFC27D, 0xF6E8C3, 0xF5F5F5, 0xC7EAE5, 0x80CDC1, 0x35978F, 0x01665E,
    0x8C510A, 0xBF812D, 0xDFC27D, 0xF6E8C3, 0xC7EAE5, 0x80CDC1, 0x35978F, 0x01665E,
    0x8C510A, 0xD8B365, 0xF6E8C3, 0xF5F5F5, 0xC7EAE5, 0x5AB4AC, 0x01665E,
    0x8C510A, 0xD8B365, 0xF6E8C3, 0xC7EAE5, 0x5AB4AC, 0x01665E,
    0xA6611A, 0xDFC27D, 0xF5F5F5, 0x80CDC1, 0x018571,
    0xA6611A, 0xDFC27D, 0x80CDC1, 0x018571,
    0xD8B365, 0xF5F5F5, 0x5AB4AC,
    // Sequential Palettes
    //   Blue-Green
    0xF7FCFD, 0xE5F5F9, 0xCCECE6, 0x99D8C9, 0x66C2A4, 0x41AE76, 0x238B45, 0x006D2C, 0x00441B,
    0xF7FCFD, 0xE5F5F9, 0xCCECE6, 0x99D8C9, 0x66C2A4, 0x41AE76, 0x238B45, 0x005824,
    0xEDF8FB, 0xCCECE6, 0xCCECE6, 0x66C2A4, 0x41AE76, 0x238B45, 0x005824,
    0xEDF8FB, 0xCCECE6, 0x99D8C9, 0x66C2A4, 0x2CA25F, 0x006D2C,
    0xEDF8FB, 0xB2E2E2, 0x66C2A4, 0x2CA25F, 0x006D2C,
    0xEDF8FB, 0xB2E2E2, 0x66C2A4, 0x238B45,
    0xE5F5F9, 0x99D8C9, 0x2CA25F,
    //   Yellow-Orange-Brown
    0xFFFFE5, 0xFFF7BC, 0xFEE391, 0xFEC44F, 0xFE9929, 0xEC7014, 0xCC4C02, 0x993404, 0x662506,
    0xFFFFE5, 0xFFF7BC, 0xFEE391, 0xFEC44F, 0xFE9929, 0xEC7014, 0xCC4C02, 0x8C2D04,
    0xFFFFD4, 0xFEE391, 0xFEC44F, 0xFE9929, 0xEC7014, 0xCC4C02, 0x8C2D04,
    0xFFFFD4, 0xFEE391, 0xFEC44F, 0xFE9929, 0xD95F0E, 0x993404,
    0xFFFFD4, 0xFED98E, 0xFE9929, 0xD95F0E, 0x993404,
    0xFFFFD4, 0xFED98E, 0xFE9929, 0xCC4C02,
    0xFFF7BC, 0xFEC44F, 0xD95F0E,
    //   Blue-Purple
    0xF7FCFD, 0xE0ECF4, 0xBFD3E6, 0x9EBCDA, 0x8C96C6, 0x8C6BB1, 0x88419D, 0x810F7C, 0x4D004B,
    0xF7FCFD, 0xE0ECF4, 0xBFD3E6, 0x9EBCDA, 0x8C96C6, 0x8C6BB1, 0x88419D, 0x6E016B,
    0xEDF8FB, 0xBFD3E6, 0x9EBCDA, 0x8C96C6, 0x8C6BB1, 0x88419D, 0x6E016B,
    0xEDF8FB, 0xBFD3E6, 0x9EBCDA, 0x8C96C6, 0x8856A7, 0x810F7C,
    0xEDF8FB, 0xB3CDE3, 0x8C96C6, 0x8856A7, 0x810F7C,
    0xEDF8FB, 0xB3CDE3, 0x8C96C6, 0x88419D,
    0xE0ECF4, 0x9EBCDA, 0x8856A7,
    // Qualitative Palettes
    //   Accent
    0x7FC97F, 0xBEAED4, 0xFDC086, 0xFFFF99, 0x386CB0, 0xF0027F, 0xBF5B17, 0x666666,
    //   Dark2
    0x1B9E77, 0xD95F02, 0x7570B3, 0xE7298A, 0x66A61E, 0xE6AB02, 0xA6761D, 0x666666,
    //   Set2
    0x66C2A5, 0xFC8D62, 0x8DA0CB, 0xE78AC3, 0xA6D854, 0xFFD92F, 0xE5C494, 0xB3B3B3,
    //   Pastel2
    0xB3E2CD, 0xFDCDAC, 0xCBD5E8, 0xF4CAE4, 0xE6F5C9, 0xFFF2AE, 0xF1E2CC, 0xCCCCCC,
    //   Pastel1
    0xFBB4AE, 0xB3CDE3, 0xCCEBC5, 0xDECBE4, 0xFED9A6, 0xFFFFCC, 0xE5D8BD, 0xFDDAEC, 0xF2F2F2,
    //   Set1
    0xE41A1C, 0x377EB8, 0x4DAF4A, 0x984EA3, 0xFF7F00, 0xFFFF33, 0xA65628, 0xF781BF, 0x999999,
    //   Paired
    0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99,
    //   Set3
    0x8DD3C7, 0xFFFFB3, 0xBEBADA, 0xFB8072, 0x80B1D3, 0xFDB462, 0xB3DE69, 0xFCCDE5, 0xD9D9D9, 0xBC80BD, 0xCCEBC5, 0xFFED6F,
    };
  const char* names[] =
    {
    "Diverging Purple-Orange",
    "Diverging Spectral",
    "Diverging Brown-Blue-Green",

    "Sequential Blue-Green",
    "Sequential Yellow-Orange-Brown",
    "Sequential Blue-Purple",

    "Qualitative Accent",
    "Qualitative Dark2",
    "Qualitative Set2",
    "Qualitative Pastel2",
    "Qualitative Pastel1",
    "Qualitative Set1",
    "Qualitative Paired",
    "Qualitative Set3"
    };
  int sizes[][2] =
    {
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
  for ( unsigned i = 0; i < sizeof(names)/sizeof(names[0]); ++ i )
    {
    int start = sizes[i][0];
    int stop  = sizes[i][1];
    int step = start > stop ? -1 : 1;
    for ( int n = start; n != stop + step; n += step )
      {
      vtksys_ios::ostringstream os;
      os << names[i] << " (" << n << ")";
      schemes->CreateScheme( os.str().c_str() );
      for ( int j = 0; j < n; ++ j )
        {
        schemes->AddColor( vtkColor4ubFromHex3( *(color ++) ) );
        }
      }
    }
}

vtkStandardNewMacro(vtkBrewerColors);
vtkBrewerColors::vtkBrewerColors()
{
  this->Storage = new vtkColorSchemeInternal;
  this->CurrentColorCache = 0;
  this->CurrentSize = -1;
  constructDefaultSchemes( this );
  this->SetCurrentScheme( "Diverging Purple-Orange (4)" );
}

vtkBrewerColors::~vtkBrewerColors()
{
  delete this->Storage;
}

void vtkBrewerColors::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Storage: " << this->Storage->size() << " entries\n";
  os << indent << "CurrentScheme: \"" << ( this->CurrentScheme.empty() ? "(EMPTY)" : this->CurrentScheme.c_str() ) << "\"\n";
  os << indent << "CurrentColorCache: " << this->CurrentColorCache << "\n";
}

vtkIdType vtkBrewerColors::GetNumberOfSchemes()
{
  return static_cast<vtkIdType>( this->Storage->size() );
}

const char* vtkBrewerColors::GetScheme( vtkIdType index )
{
  if ( index < 0 || index > this->GetNumberOfSchemes() )
    {
    return 0;
    }

  vtkColorSchemeInternal::iterator it = this->Storage->begin();
  for ( ; ( index != 0 ) && ( it != this->Storage->end() ) ; -- index )
    {
    ++ it ;
    }
  if ( it == this->Storage->end() )
    {
    return 0;
    }
  return it->first.c_str();
}

void vtkBrewerColors::SetCurrentScheme( const char* schemeName )
{
  if ( this->CurrentScheme == schemeName || ! schemeName || schemeName[0] == '\0' )
    {
    return;
    }
  vtkColorSchemeInternal::iterator it = this->Storage->find( schemeName );
  if ( it == this->Storage->end() )
    {
    return;
    }
  this->CurrentScheme = it->first;
  this->CurrentColorCache = &it->second[0];
  this->CurrentSize = static_cast<vtkIdType>( it->second.size() );
}

bool vtkBrewerColors::CreateScheme( const char* schemeName )
{
  if ( ! schemeName )
    { // bad name
    return false;
    }
  vtkColorSchemeInternal::iterator it = this->Storage->find( schemeName );
  if ( it != this->Storage->end() )
    { // already exists
    return false;
    }
  std::vector<vtkColor4ub> blank;
  (*this->Storage)[schemeName] = blank;
  this->SetCurrentScheme( schemeName );
  return true;
}

int vtkBrewerColors::GetNumberOfColors()
{
  return this->CurrentSize;
}

vtkColor4ub vtkBrewerColors::GetColor( int index ) const
{
  if ( this->CurrentColorCache && index >= 0 && index < this->CurrentSize )
    return this->CurrentColorCache[index];
  return vtkColor4ub();
}

vtkColor4ub vtkBrewerColors::GetColorRepeating( int index ) const
{
  return this->GetColor( index % this->CurrentSize );
}

void vtkBrewerColors::SetColor( int index, const vtkColor4ub& color )
{
  if ( this->CurrentSize >= 0 && index >= 0 && index < this->CurrentSize )
    {
    this->CurrentColorCache[index] = color;
    }
  else
    {
    vtkErrorMacro( "No current scheme or index " << index << " out of range." );
    }
}

void vtkBrewerColors::AddColor( const vtkColor4ub& color )
{
  if ( this->CurrentSize >= 0 )
    {
    (*this->Storage)[this->CurrentScheme].push_back( color );
    this->CurrentColorCache = &(*this->Storage)[this->CurrentScheme][0];
    this->CurrentSize = static_cast<vtkIdType>( (*this->Storage)[this->CurrentScheme].size() );
    }
  else
    {
    vtkErrorMacro( "No current scheme to which to add color." );
    }
}

void vtkBrewerColors::InsertColor( int index, const vtkColor4ub& color )
{
  if ( this->CurrentSize >= 0 && index >= 0 && index < this->CurrentSize )
    {
    vtkColorSchemeInternal::iterator scheme = this->Storage->find( this->CurrentScheme );
    scheme->second.insert( scheme->second.begin() + index, color );
    this->CurrentColorCache = &scheme->second[0];
    this->CurrentSize = static_cast<vtkIdType>( scheme->second.size() );
    }
  else
    {
    vtkErrorMacro( "No current scheme or index " << index << " out of range." );
    }
}

void vtkBrewerColors::RemoveColor( int index )
{
  if ( this->CurrentSize >= 0 && index >= 0 && index < this->CurrentSize )
    {
    vtkColorSchemeInternal::iterator scheme = this->Storage->find( this->CurrentScheme );
    scheme->second.erase( scheme->second.begin() + index );
    this->CurrentColorCache = &scheme->second[0];
    this->CurrentSize = static_cast<vtkIdType>( scheme->second.size() );
    }
  else
    {
    vtkErrorMacro( "No current scheme or index " << index << " out of range." );
    }
}

void vtkBrewerColors::ClearColors()
{
  if ( this->CurrentSize >= 0 )
    {
    (*this->Storage)[this->CurrentScheme].clear();
    this->CurrentSize = 0;
    }
}

void vtkBrewerColors::DeepCopy( vtkBrewerColors* other )
{
  if ( other )
    {
    *this->Storage = *other->Storage;
    }
  this->CurrentScheme = other->CurrentScheme.empty() ? "foo" : ""; // Make sure the next line does the right thing:
  this->SetCurrentScheme( other->CurrentScheme );
}

vtkLookupTable* vtkBrewerColors::CreateLookupTable( vtkLookupTable* lkup )
{
  if ( ! lkup )
    lkup = vtkLookupTable::New();
  if ( lkup )
    {
    lkup->SetNumberOfTableValues( this->GetNumberOfColors() );
    lkup->IndexedLookupOn();
    for ( int i = 0; i < this->GetNumberOfColors(); ++ i )
      {
      vtkColor4ub colr = this->GetColor( i );
      lkup->SetTableValue( i, colr.Red()/255., colr.Green()/255., colr.Blue()/255., colr.Alpha()/255. );
      }
    }
  return lkup;
}
