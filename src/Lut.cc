//
// Methods for lookup table
//
#include <math.h>
#include "Lut.hh"

vlLookupTable::vlLookupTable(int sze, int ext)
{
  this->NumColors = sze;
  this->Table.Initialize(sze,ext);

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
  this->Modified();
  this->NumColors = sz;
  return this->Table.Initialize(this->NumColors,ext);
}

void  vlLookupTable::SetTableRange(float r[2])
{
  this->TableRange[0] = r[0];
  this->TableRange[1] = r[1];
}
void  vlLookupTable::SetTableRange(float min, float max)
{
  this->TableRange[0] = min;
  this->TableRange[1] = max;
}

void vlLookupTable::Build()
{
  int i, hueCase, indx;
  float hue, sat, val, lx, ly, lz, frac, hinc, sinc, vinc;
  float rgb[3];

  if ( this->Table.NumColors() < 1 ||
  (this->Mtime > this->BuildTime && this->InsertTime < this->BuildTime) )
    {
    hinc = (this->HueRange[1] - this->HueRange[0])/(this->NumColors-1);
    sinc = (this->SaturationRange[1] - this->SaturationRange[0])/(this->NumColors-1);
    vinc = (this->ValueRange[1] - this->ValueRange[0])/(this->NumColors-1);

    for (i=0; i < this->NumColors; i++) 
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

      this->Table.SetColor(i,rgb);
    }
  this->BuildTime.Modified();
  }
}

float *vlLookupTable::MapValue(float v)
{
  int indx;

  indx = (v-this->TableRange[0])/(this->TableRange[1]-this->TableRange[0]) * this->NumColors;
  indx = (indx < 0 ? 0 : (indx >= this->NumColors ? this->NumColors-1 : indx));

  return this->Table.GetColor(indx);
}

void vlLookupTable::SetTableValue (int indx, float rgb[3])
{
  indx = (indx < 0 ? 0 : (indx >= this->NumColors ? this->NumColors-1 : indx));
  this->Table.SetColor(indx,rgb);
  this->InsertTime.Modified();
  this->Modified();
}

float *vlLookupTable::GetTableValue (int indx)
{
  indx = (indx < 0 ? 0 : (indx >= this->NumColors ? this->NumColors-1 : indx));
  return this->Table.GetColor(indx);
  
}

