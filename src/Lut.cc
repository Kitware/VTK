/*=========================================================================

  Program:   Visualization Library
  Module:    Lut.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <math.h>
#include "Lut.hh"

// Description:
// Construct with range=(0,1); and hsv ranges set up for rainbow color table.
vlLookupTable::vlLookupTable(int sze, int ext)
{
  this->NumberOfColors = sze;
  this->Table.Allocate(sze,ext);

  this->TableRange[0] = 0.0;
  this->TableRange[1] = 1.0;

  this->HueRange[0] = 0.0;
  this->HueRange[1] = 0.66667;

  this->SaturationRange[0] = 1.0;
  this->SaturationRange[1] = 1.0;

  this->ValueRange[0] = 1.0;
  this->ValueRange[1] = 1.0;

  this->AlphaRange[0] = 1.0;
  this->AlphaRange[1] = 1.0;

};

// Description:
// Allocate a color table of specified size.
int vlLookupTable::Allocate(int sz, int ext) 
{
  this->Modified();
  this->NumberOfColors = sz;
  return this->Table.Allocate(this->NumberOfColors,ext);
}

// Description:
// Set the minimum/maximum scalar values for scalar mapping. Scalar values
// less than minimum range value are clamped to minimum range value.
// Scalar values greater than maximum range value are clamped to maximum
// range value.
void  vlLookupTable::SetTableRange(float r[2])
{
  this->SetTableRange(r[0],r[1]);
}

// Description:
// Set the minimum/maximum scalar values for scalar mapping. Scalar values
// less than minimum range value are clamped to minimum range value.
// Scalar values greater than maximum range value are clamped to maximum
// range value.
void  vlLookupTable::SetTableRange(float min, float max)
{
  if ( min >= max )
    {
    vlErrorMacro (<<"Bad table range");
    return;
    }

  this->TableRange[0] = min;
  this->TableRange[1] = max;
}

// Description:
// Generate lookup table from hue, saturation, value, alpha min/max values. 
// Table is built from linear ramp of each value.
void vlLookupTable::Build()
{
  int i, hueCase;
  float hue, sat, val, lx, ly, lz, frac, hinc, sinc, vinc, ainc;
  float rgba[4], alpha;
  unsigned char c_rgba[4];

  if ( this->Table.GetNumberOfColors() < 1 ||
  (this->GetMTime() > this->BuildTime && this->InsertTime < this->BuildTime) )
    {
    hinc = (this->HueRange[1] - this->HueRange[0])/(this->NumberOfColors-1);
    sinc = (this->SaturationRange[1] - this->SaturationRange[0])/(this->NumberOfColors-1);
    vinc = (this->ValueRange[1] - this->ValueRange[0])/(this->NumberOfColors-1);
    ainc = (this->AlphaRange[1] - this->AlphaRange[0])/(this->NumberOfColors-1);

    for (i=0; i < this->NumberOfColors; i++) 
      {
      hue = this->HueRange[0] + i * hinc;
      sat = this->SaturationRange[0] + i * sinc;
      val = this->ValueRange[0] + i * vinc;
      alpha = this->AlphaRange[0] + i * ainc;

      hueCase = (int)(hue * 6);
      frac = 6*hue - hueCase;
      lx = val*(1.0 - sat);
      ly = val*(1.0 - sat*frac);
      lz = val*(1.0 - sat*(1.0 - frac));

      switch (hueCase) 
      {

        /* 0<hue<1/6 */
      case 0:
      case 6:
        rgba[0] = val;
        rgba[1] = lz;
        rgba[2] = lx;
        break;
        /* 1/6<hue<2/6 */
      case 1:
        rgba[0] = ly;
        rgba[1] = val;
        rgba[2] = lx;
        break;
        /* 2/6<hue<3/6 */
      case 2:
        rgba[0] = lx;
        rgba[1] = val;
        rgba[2] = lz;
        break;
        /* 3/6<hue/4/6 */
      case 3:
        rgba[0] = lx;
        rgba[1] = ly;
        rgba[2] = val;
        break;
        /* 4/6<hue<5/6 */
      case 4:
        rgba[0] = lz;
        rgba[1] = lx;
        rgba[2] = val;
        break;
        /* 5/6<hue<1 */
      case 5:
        rgba[0] = val;
        rgba[1] = lx;
        rgba[2] = ly;
        break;
      }

      c_rgba[0] = (unsigned char) 
        ((float)127.5*(1.0+(float)cos((1.0-(double)rgba[0])*3.141593)));
      c_rgba[1] = (unsigned char)
        ((float)127.5*(1.0+(float)cos((1.0-(double)rgba[1])*3.141593)));
      c_rgba[2] = (unsigned char)
        ((float)127.5*(1.0+(float)cos((1.0-(double)rgba[2])*3.141593)));
      c_rgba[3] = (unsigned char) (alpha*255.0);

      this->Table.InsertColor(i,c_rgba);
    }
  }
  this->BuildTime.Modified();
}

// Description:
// Given a scalar value v, return an rgba color value from lookup table.
unsigned char *vlLookupTable::MapValue(float v)
{
  int indx;

  indx = (int)((v-this->TableRange[0])/(this->TableRange[1]-this->TableRange[0]) * this->NumberOfColors);
  indx = (indx < 0 ? 0 : (indx >= this->NumberOfColors ? this->NumberOfColors-1 : indx));

  return this->Table.GetColor(indx);
}

// Description:
// Directly load color into lookup table
void vlLookupTable::SetTableValue (int indx, unsigned char rgba[4])
{
  indx = (indx < 0 ? 0 : (indx >= this->NumberOfColors ? this->NumberOfColors-1 : indx));
  this->Table.SetColor(indx,rgba);
  this->InsertTime.Modified();
  this->Modified();
}

// Description:
// Directly load color into lookup table
void vlLookupTable::SetTableValue (int indx, unsigned char r, unsigned char g,
                                   unsigned char b, unsigned char a)
{
  unsigned char rgba[4];
  rgba[0] = r; rgba[1] = g; rgba[2] = b; rgba[4] = a;
  this->SetTableValue(indx,rgba);
}

// Description:
// Return a rgba color value for the given index into the lookup table.
unsigned char *vlLookupTable::GetTableValue (int indx)
{
  indx = (indx < 0 ? 0 : (indx >= this->NumberOfColors ? this->NumberOfColors-1 : indx));
  return this->Table.GetColor(indx);
  
}

void vlLookupTable::PrintSelf(ostream& os, vlIndent indent)
{
  vlObject::PrintSelf(os,indent);

  os << indent << "Build Time: " <<this->BuildTime.GetMTime() << "\n";
  os << indent << "Hue Range: (" << this->HueRange[0] << ", "
     << this->HueRange[1] << ")\n";
  os << indent << "Insert Time: " <<this->InsertTime.GetMTime() << "\n";
  os << indent << "Number Of Colors: " << this->GetNumberOfColors() << "\n";
  os << indent << "Saturation Range: (" << this->SaturationRange[0] << ", "
     << this->SaturationRange[1] << ")\n";
  os << indent << "Table Range: (" << this->TableRange[0] << ", "
     << this->TableRange[1] << ")\n";
  os << indent << "Value Range: (" << this->ValueRange[0] << ", "
     << this->ValueRange[1] << ")\n";
}
