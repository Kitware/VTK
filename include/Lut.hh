/*=========================================================================

  Program:   Visualization Library
  Module:    Lut.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
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
  int Allocate(int sz=256, int ext=256);
  void Build();
  char *GetClassName() {return "vlLookupTable";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(NumberOfColors,int,8, 65536);
  vlGetMacro(NumberOfColors,int);

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
  void SetTableValue (int indx, float r, float g, float b);
  float *GetTableValue (int);

protected:
  int NumberOfColors;
  vlRGBArray Table;  
  float TableRange[2];
  float HueRange[2];
  float SaturationRange[2];
  float ValueRange[2];
  vlTimeStamp InsertTime;
  vlTimeStamp BuildTime;
};

#endif


