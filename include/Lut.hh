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
// .NAME vlLookupTable - map scalar values into colors or colors to scalars; generate color table
// .SECTION Description
// vlLookupTable is an object that is used by mapper objects to map scalar 
// values into rgb color specification, or rgb into scalar values. The 
// color table can be created by direct insertion of color values, or by 
// specifying  hue, saturation, and value range and generating a table.
// .SECTION Caveats
//    vlLookupTable is a reference counted object. Therefore you should 
// always use operator "new" to construct new objects. This procedure will
// avoid memory problems (see text).

#ifndef __vlLookupTable_h
#define __vlLookupTable_h

#include "RefCount.hh"
#include "Pixmap.hh"

class vlLookupTable : public vlRefCount
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

  unsigned char *MapValue(float v);
  void SetTableValue (int indx, unsigned char rgb[3]);
  void SetTableValue (int indx, unsigned char r, unsigned char g, unsigned char b);
  unsigned char *GetTableValue (int id);
  void GetTableValue (int id, unsigned char rgb[3]);

protected:
  int NumberOfColors;
  vlPixmap Table;  
  float TableRange[2];
  float HueRange[2];
  float SaturationRange[2];
  float ValueRange[2];
  vlTimeStamp InsertTime;
  vlTimeStamp BuildTime;
};

#endif


