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
  this->TableRange[0] = r[0];
  this->TableRange[1] = r[1];
}

// Description:
// Set the minimum/maximum scalar values for scalar mapping. Scalar values
// less than minimum range value are clamped to minimum range value.
// Scalar values greater than maximum range value are clamped to maximum
// range value.
void  vlLookupTable::SetTableRange(float min, float max)
{
  this->TableRange[0] = min;
  this->TableRange[1] = max;
}

// Description:
// Generate lookup table from object parameters.
void vlLookupTable::Build()
{
  int i, hueCase;
  float hue, sat, val, lx, ly, lz, frac, hinc, sinc, vinc;
  float rgb[3];

  if ( this->Table.NumberOfColors() < 1 ||
  (this->GetMTime() > this->BuildTime && this->InsertTime < this->BuildTime) )
    {
    hinc = (this->HueRange[1] - this->HueRange[0])/(this->NumberOfColors-1);
    sinc = (this->SaturationRange[1] - this->SaturationRange[0])/(this->NumberOfColors-1);
    vinc = (this->ValueRange[1] - this->ValueRange[0])/(this->NumberOfColors-1);

    for (i=0; i < this->NumberOfColors; i++) 
      {
      hue = this->HueRange[0] + i * hinc;
      sat = this->SaturationRange[0] + i * sinc;
      val = this->ValueRange[0] + i * vinc;

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
        rgb[0] = val;
        rgb[1] = lz;
        rgb[2] = lx;
        break;
        /* 1/6<hue<2/6 */
      case 1:
        rgb[0] = ly;
        rgb[1] = val;
        rgb[2] = lx;
        break;
        /* 2/6<hue<3/6 */
      case 2:
        rgb[0] = lx;
        rgb[1] = val;
        rgb[2] = lz;
        break;
        /* 3/6<hue/4/6 */
      case 3:
        rgb[0] = lx;
        rgb[1] = ly;
        rgb[2] = val;
        break;
        /* 4/6<hue<5/6 */
      case 4:
        rgb[0] = lz;
        rgb[1] = lx;
        rgb[2] = val;
        break;
        /* 5/6<hue<1 */
      case 5:
        rgb[0] = val;
        rgb[1] = lx;
        rgb[2] = ly;
        break;
      }

      rgb[0] = (1.0+(float)cos((1.0-(double)rgb[0])*3.141593))/2.0;
      rgb[1] = (1.0+(float)cos((1.0-(double)rgb[1])*3.141593))/2.0;
      rgb[2] = (1.0+(float)cos((1.0-(double)rgb[2])*3.141593))/2.0;

      this->Table.InsertColor(i,rgb);
    }
  }
  this->BuildTime.Modified();
}

// Description:
// Given a scalar value v, return an r-g-b color value from lookup table.
float *vlLookupTable::MapValue(float v)
{
  int indx;

  indx = (int)((v-this->TableRange[0])/(this->TableRange[1]-this->TableRange[0]) * this->NumberOfColors);
  indx = (indx < 0 ? 0 : (indx >= this->NumberOfColors ? this->NumberOfColors-1 : indx));

  return this->Table.GetColor(indx);
}

// Description:
// Directly load color into lookup table
void vlLookupTable::SetTableValue (int indx, float rgb[3])
{
  indx = (indx < 0 ? 0 : (indx >= this->NumberOfColors ? this->NumberOfColors-1 : indx));
  this->Table.SetColor(indx,rgb);
  this->InsertTime.Modified();
  this->Modified();
}

// Description:
// Directly load color into lookup table
void vlLookupTable::SetTableValue (int indx, float r, float g, float b)
{
  float rgb[3];
  rgb[0] = r; rgb[1] = g; rgb[2] = b;
  vlLookupTable::SetTableValue(indx,rgb);
}

// Description:
// Return a r-g-b color value for the given index into the lookup table.
float *vlLookupTable::GetTableValue (int indx)
{
  indx = (indx < 0 ? 0 : (indx >= this->NumberOfColors ? this->NumberOfColors-1 : indx));
  return this->Table.GetColor(indx);
  
}

void vlLookupTable::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlLookupTable::GetClassName()))
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
}
