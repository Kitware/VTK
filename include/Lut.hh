//
// LookupTable takes PolyData as input
//
#ifndef __vlLookupTable_h
#define __vlLookupTable_h

#include "Object.hh"
#include "RGBArray.hh"

class vlLookupTable : public vlObject 
{
public:
  vlLookupTable();
  int Initialize(const int sz=256, const int ext=256);
  void SetTableRange(float min, float max);
  void GetTableRange(float &min,float &max);
  int GetTableSize();
  void SetHueRange(float min, float max);
  void GetHueRange(float &min,float &max);
  void SetSaturationRange(float min, float max);
  void GetSaturationRange(float &min,float &max);
  void SetValueRange(float min, float max);
  void GetValueRange(float &min,float &max);
  void Build();
  vlRGBColor &MapValue(float v);
  void SetTableValue (int indx, vlRGBColor &rgb_c);
  vlRGBColor &GetTableValue (int);

protected:
  vlRGBArray Table;  
  float TableRange[2];
  float HueRange[2];
  float SaturationRange[2];
  float ValueRange[2];

};

#endif


