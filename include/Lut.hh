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
  int GetTableSize();
  void Build();

  vlSetVector2Macro(TableRange,float);
  vlGetVectorMacro(TableRange,float);

  vlSetVector2Macro(HueRange,float);
  vlGetVectorMacro(HueRange,float);

  vlSetVector2Macro(SaturationRange,float);
  vlGetVectorMacro(SaturationRange,float);

  vlSetVector2Macro(ValueRange,float);
  vlGetVectorMacro(ValueRange,float);

  float *MapValue(float v);
  void SetTableValue (int indx, float rgb[3]);
  float *GetTableValue (int);

protected:
  vlRGBArray Table;  
  float TableRange[2];
  float HueRange[2];
  float SaturationRange[2];
  float ValueRange[2];

};

#endif


