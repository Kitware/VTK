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
  vlLookupTable(int sze=256, int ext=256);
  int Initialize(int sz=256, int ext=256);
  void Build();

  vlSetClampMacro(NumColors,int,8, 65536);
  vlGetMacro(NumColors,int);

  void SetTableRange(float r[2]); // can't use macro 'cause don't want modified
  void SetTableRange(float min, float max);
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
  int NumColors;
  vlRGBArray Table;  
  float TableRange[2];
  float HueRange[2];
  float SaturationRange[2];
  float ValueRange[2];
  vlTimeStamp InsertTime;
  vlTimeStamp BuildTime;
};

#endif


