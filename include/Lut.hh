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
// .NAME vlLookupTable - map scalar values into colors; generate color tables
// .SECTION Description
// vlLookupTable is an object that is used by mapper objects to map scalar 
// values into RGB color specification. The color table can be created by 
// direct insertion of color values, or by specifying  hue, saturation, and 
// value range and generating a table.

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

  // Description:
  // Set the number of colors in the lookup table.
  vlSetClampMacro(NumberOfColors,int,8, 65536);
  vlGetMacro(NumberOfColors,int);

  void SetTableRange(float r[2]); // can't use macro 'cause don't want modified
  void SetTableRange(float min, float max);
  vlGetVectorMacro(TableRange,float,2);

  // Description:
  // Set the range in hue (using automatic generation).
  vlSetVector2Macro(HueRange,float);
  vlGetVectorMacro(HueRange,float,2);

  // Description:
  // Set the range in saturation (using automatic generation).
  vlSetVector2Macro(SaturationRange,float);
  vlGetVectorMacro(SaturationRange,float,2);

  // Description:
  // Set the range in value (using automatic generation).
  vlSetVector2Macro(ValueRange,float);
  vlGetVectorMacro(ValueRange,float,2);

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


