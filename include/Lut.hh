/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Lut.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkLookupTable - map scalar values into colors or colors to scalars; generate color table
// .SECTION Description
// vtkLookupTable is an object that is used by mapper objects to map scalar 
// values into rgba (red-green-blue-alpha transparency) color specification, 
// or rgba into scalar values. The color table can be created by direct 
// insertion of color values, or by specifying  hue, saturation, value, and 
// alpha range and generating a table.
// .SECTION Caveats
//    vtkLookupTable is a reference counted object. Therefore you should 
// always use operator "new" to construct new objects. This procedure will
// avoid memory problems (see text).

#ifndef __vtkLookupTable_h
#define __vtkLookupTable_h

#include "RefCount.hh"
#include "APixmap.hh"

class vtkLookupTable : public vtkRefCount
{
public:
  vtkLookupTable(int sze=256, int ext=256);
  int Allocate(int sz=256, int ext=256);
  void Build();
  char *GetClassName() {return "vtkLookupTable";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of colors in the lookup table.
  vtkSetClampMacro(NumberOfColors,int,8, 65536);
  vtkGetMacro(NumberOfColors,int);

  void SetTableRange(float r[2]); // can't use macro 'cause don't want modified
  void SetTableRange(float min, float max);
  vtkGetVectorMacro(TableRange,float,2);

  // Description:
  // Set the range in hue (using automatic generation). Hue ranges from (0,1).
  vtkSetVector2Macro(HueRange,float);
  vtkGetVectorMacro(HueRange,float,2);

  // Description:
  // Set the range in saturation (using automatic generation). Hue ranges from
  // (0,1).
  vtkSetVector2Macro(SaturationRange,float);
  vtkGetVectorMacro(SaturationRange,float,2);

  // Description:
  // Set the range in value (using automatic generation). Value ranges from
  // (0,1).
  vtkSetVector2Macro(ValueRange,float);
  vtkGetVectorMacro(ValueRange,float,2);

  // Description:
  // Set the range in alpha (using automatic generation). Alpha ranges from 
  // (0,1).
  vtkSetVector2Macro(AlphaRange,float);
  vtkGetVectorMacro(AlphaRange,float,2);

  unsigned char *MapValue(float v);

  void SetTableValue (int indx, float rgba[4]);
  void SetTableValue (int indx, float r, float g, float b, float a=1.0);

  float *GetTableValue (int id);
  void GetTableValue (int id, float rgba[4]);

  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);
  void WrotePtr();

protected:
  int NumberOfColors;
  vtkAPixmap Table;  
  float TableRange[2];
  float HueRange[2];
  float SaturationRange[2];
  float ValueRange[2];
  float AlphaRange[2];
  vtkTimeStamp InsertTime;
  vtkTimeStamp BuildTime;
};

// Description:
// Get pointer to color table data. Format is array of unsigned char
// r-g-b-a-r-g-b-a...
inline unsigned char *vtkLookupTable::GetPtr(const int id)
{
  return this->Table.GetPtr(id);
}
// Description:
// Get pointer to data. Useful for direct writes into object. MaxId is bumped
// by number (and memory allocated if necessary). Id is the location you 
// wish to write into; number is the number of rgba values to write.
// Use the method WrotePtr() to mark completion of write.
inline unsigned char *vtkLookupTable::WritePtr(const int id, const int number)
{
  return this->Table.WritePtr(id,number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkLookupTable::WrotePtr() {}

#endif


