/*=========================================================================

  Program:   Visualization Library
  Module:    LogLut.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkLogLookupTable - map scalar values into colors using logarithmic (base 10) color table
// .SECTION Description
// vtkLogLookupTable is an object that is used by mapper objects to map scalar 
// values into rgba (red-green-blue-alpha transparency) color specification, 
// or rgba into scalar values. The difference between this class and its
// superclass vtkLookupTable is that this class performs scalar mapping based
// on a logarithmic lookup process. (Uses log base 10).
//    If non-positive ranges are encountered, then they are converted to 
// positive values using absolute value.
// .SECTION See Also
// vtkLookupTable

#ifndef __vtkLogLookupTable_h
#define __vtkLogLookupTable_h

#include "Lut.hh"

class vtkLogLookupTable : public vtkLookupTable
{
public:
  vtkLogLookupTable(int sze=256, int ext=256);
  char *GetClassName() {return "vtkLogLookupTable";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetTableRange(float min, float max);
  unsigned char *MapValue(float v);

protected:
  float LogMinRange;
  float LogMaxRange;
  float UseAbsoluteValue;
};

#endif
