//
// Methods for lookup table
//
#include <math.h>
#include "Lut.h"

LookupTable::LookupTable()
{
  tableRange[0] = 0.0;
  tableRange[1] = 1.0;

  hueRange[0] = 0.0;
  hueRange[1] = 0.66667;

  saturationRange[0] = 1.0;
  saturationRange[1] = 1.0;

  valueRange[0] = 1.0;
  valueRange[1] = 1.0;
};

int LookupTable::Initialize(const int sz, const int ext) 
{
  return table.Initialize(sz,ext);
}

void LookupTable::setTableRange(float min, float max)
{
  if ( tableRange[0] != min || tableRange[1] != max ) {
    tableRange[0] = min;
    tableRange[1] = min;
    modified();
  }
}

void LookupTable::getTableRange(float &min,float &max)
{
  min = tableRange[0];
  max = tableRange[1];
}

int LookupTable::getTableSize()
{
  return table.numColors();
}

void LookupTable::setHueRange(float min, float max)
{
  min = (min < 0.0 ? 0.0 : (min > 1.0 ? 1.0 : min));
  max = (max < 0.0 ? 0.0 : (max > 1.0 ? 1.0 : max));

  if ( hueRange[0] != min || hueRange[1] != max ) {
    hueRange[0] = min;
    hueRange[1] = max;
    modified();
  }
}

void LookupTable::getHueRange(float &min,float &max)
{
  min = hueRange[0];
  max = hueRange[1];
}

void LookupTable::setSaturationRange(float min, float max)
{
  min = (min < 0.0 ? 0.0 : (min > 1.0 ? 1.0 : min));
  max = (max < 0.0 ? 0.0 : (max > 1.0 ? 1.0 : max));

  if ( saturationRange[0] != min || saturationRange[1] != max ) {
    saturationRange[0] = min;
    saturationRange[1] = max;
    modified();
  }
}

void LookupTable::getSaturationRange(float &min,float &max)
{
  min = saturationRange[0];
  max = saturationRange[1];
}

void LookupTable::setValueRange(float min, float max)
{
  min = (min < 0.0 ? 0.0 : (min > 1.0 ? 1.0 : min));
  max = (max < 0.0 ? 0.0 : (max > 1.0 ? 1.0 : max));

  if ( valueRange[0] != min || valueRange[1] != max ) {
    valueRange[0] = min;
    valueRange[1] = max;
    modified();
  }
}

void LookupTable::getValueRange(float &min,float &max)
{
  min = valueRange[0];
  max = valueRange[1];
}

void LookupTable::build()
{
  int i, hue_case, indx, numColors;
  float hue, sat, val, lx, ly, lz, frac, hinc, sinc, vinc;
  RGBColor rgb;

  if ( table.numColors() < 1 )
  {
    Initialize();
  }
  numColors = table.numColors();

  hinc = (hueRange[1] - hueRange[0])/(numColors-1);
  sinc = (saturationRange[1] - saturationRange[0])/(numColors-1);
  vinc = (valueRange[1] - valueRange[0])/(numColors-1);
  
  for (i=0; i < numColors; i++) {
    hue = hueRange[0] + i * hinc;
    sat = saturationRange[0] + i * sinc;
    val = valueRange[0] + i * vinc;
    
    hue_case = (int)(hue * 6);
    frac = 6*hue - hue_case;
    lx = val*(1.0 - sat);
    ly = val*(1.0 - sat*frac);
    lz = val*(1.0 - sat*(1.0 - frac));
    
    switch (hue_case) {
      
      /* 0<hue<1/6 */
    case 0:
    case 6:
      rgb.x[0] = val;
      rgb.x[1] = lz;
      rgb.x[2] = lx;
      break;
      /* 1/6<hue<2/6 */
    case 1:
      rgb.x[0] = ly;
      rgb.x[1] = val;
      rgb.x[2] = lx;
      break;
      /* 2/6<hue<3/6 */
    case 2:
      rgb.x[0] = lx;
      rgb.x[1] = val;
      rgb.x[2] = lz;
      break;
      /* 3/6<hue/4/6 */
    case 3:
      rgb.x[0] = lx;
      rgb.x[1] = ly;
      rgb.x[2] = val;
      break;
      /* 4/6<hue<5/6 */
    case 4:
      rgb.x[0] = lz;
      rgb.x[1] = lx;
      rgb.x[2] = val;
      break;
      /* 5/6<hue<1 */
    case 5:
      rgb.x[0] = val;
      rgb.x[1] = lx;
      rgb.x[2] = ly;
      break;
    }
    
    rgb.x[0] = (1.0+(float)cos((1.0-(double)rgb.x[0])*3.141593))/2.0;
    rgb.x[1] = (1.0+(float)cos((1.0-(double)rgb.x[1])*3.141593))/2.0;
    rgb.x[2] = (1.0+(float)cos((1.0-(double)rgb.x[2])*3.141593))/2.0;

    table[i] = rgb;
  }
  
  buildTime.modified();
}

RGBColor &LookupTable::mapValue(float v)
{
  int indx, numColors=table.numColors();

  indx = (int) (v-tableRange[0])/(tableRange[1]-tableRange[0]) * numColors;
  indx = (indx < 0 ? 0 : (indx >= numColors ? numColors-1 : indx));

  return table[indx];
}

void LookupTable::setTableValue (int indx, RGBColor &rgb_c)
{
  int numColors=table.numColors();

  indx = (indx < 0 ? 0 : (indx >= numColors ? numColors-1 : indx));
  table[indx] = rgb_c;
}

RGBColor &LookupTable::getTableValue (int indx)
{
  int numColors=table.numColors();

  indx = (indx < 0 ? 0 : (indx >= numColors ? numColors-1 : indx));
  return table[indx];
  
}

