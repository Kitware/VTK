//
// Methods for lookup table
//
#include <math.h>
#include "Lut.hh"

vlLookupTable::vlLookupTable()
{
  this->TableRange[0] = 0.0;
  this->TableRange[1] = 1.0;

  this->HueRange[0] = 0.0;
  this->HueRange[1] = 0.66667;

  this->SaturationRange[0] = 1.0;
  this->SaturationRange[1] = 1.0;

  this->ValueRange[0] = 1.0;
  this->ValueRange[1] = 1.0;
};

int vlLookupTable::Initialize(const int sz, const int ext) 
{
  return this->Table.Initialize(sz,ext);
}

int vlLookupTable::GetTableSize()
{
  return this->Table.NumColors();
}

void vlLookupTable::Build()
{
  int i, hueCase, indx, numColors;
  float hue, sat, val, lx, ly, lz, frac, hinc, sinc, vinc;
  vlRGBColor rgb;

  if ( this->Table.NumColors() < 1 )
    {
    this->Initialize();
    }
  numColors = this->Table.NumColors();

  hinc = (this->HueRange[1] - this->HueRange[0])/(numColors-1);
  sinc = (this->SaturationRange[1] - this->SaturationRange[0])/(numColors-1);
  vinc = (this->ValueRange[1] - this->ValueRange[0])/(numColors-1);
  
  for (i=0; i < numColors; i++) 
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
      rgb.X[0] = val;
      rgb.X[1] = lz;
      rgb.X[2] = lx;
      break;
      /* 1/6<hue<2/6 */
    case 1:
      rgb.X[0] = ly;
      rgb.X[1] = val;
      rgb.X[2] = lx;
      break;
      /* 2/6<hue<3/6 */
    case 2:
      rgb.X[0] = lx;
      rgb.X[1] = val;
      rgb.X[2] = lz;
      break;
      /* 3/6<hue/4/6 */
    case 3:
      rgb.X[0] = lx;
      rgb.X[1] = ly;
      rgb.X[2] = val;
      break;
      /* 4/6<hue<5/6 */
    case 4:
      rgb.X[0] = lz;
      rgb.X[1] = lx;
      rgb.X[2] = val;
      break;
      /* 5/6<hue<1 */
    case 5:
      rgb.X[0] = val;
      rgb.X[1] = lx;
      rgb.X[2] = ly;
      break;
    }
    
    rgb.X[0] = (1.0+(float)cos((1.0-(double)rgb.X[0])*3.141593))/2.0;
    rgb.X[1] = (1.0+(float)cos((1.0-(double)rgb.X[1])*3.141593))/2.0;
    rgb.X[2] = (1.0+(float)cos((1.0-(double)rgb.X[2])*3.141593))/2.0;

    this->Table[i] = rgb;
  }
  this->Modified();
}

vlRGBColor &vlLookupTable::MapValue(float v)
{
  int indx, numColors=this->Table.NumColors();

  indx = (int) (v-this->TableRange[0])/(this->TableRange[1]-this->TableRange[0]) * numColors;
  indx = (indx < 0 ? 0 : (indx >= numColors ? numColors-1 : indx));

  return this->Table[indx];
}

void vlLookupTable::SetTableValue (int indx, vlRGBColor &rgb_c)
{
  int numColors=this->Table.NumColors();

  indx = (indx < 0 ? 0 : (indx >= numColors ? numColors-1 : indx));
  this->Table[indx] = rgb_c;
}

vlRGBColor &vlLookupTable::GetTableValue (int indx)
{
  int numColors=this->Table.NumColors();

  indx = (indx < 0 ? 0 : (indx >= numColors ? numColors-1 : indx));
  return this->Table[indx];
  
}

