/*=========================================================================

  Program:   Visualization Library
  Module:    LogLut.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <math.h>
#include "LogLut.hh"

// Description:
// Construct with effective range 1->10 (based on logarithmic values.
vtkLogLookupTable::vtkLogLookupTable(int sze, int ext):
vtkLookupTable(sze,ext)
{
  this->LogMinRange = 0.0;
  this->LogMaxRange = 1.0;
  this->UseAbsoluteValue = 0;
};

// Description:
// Set the minimum/maximum scalar values for scalar mapping. Scalar values
// less than minimum range value are clamped to minimum range value.
// Scalar values greater than maximum range value are clamped to maximum
// range value. (The log base 10 of these values is taken and mapping is
// performed in logarithmic space).
void  vtkLogLookupTable::SetTableRange(float min, float max)
{
  if ( min >= max )
    {
    vtkErrorMacro (<<"Minimum value must be less than maximum value");
    return;
    }

  this->TableRange[0] = min;
  this->TableRange[1] = max;

  if ( max >= 0.0 && min <= 0.0 )
    {
    vtkErrorMacro(<<"Can't use logarithmic table on mixed negative/positive values");
    }
  else if ( max <= 0.0 ) // okay, all negative values
    {
    if ( max == 0.0 ) max = 1.0e-06 * (min - max);
    this->UseAbsoluteValue = 1;
    this->LogMinRange = log10((double)(-min));
    this->LogMaxRange = log10((double)(-max));
    }
  else 
    {
    if ( min == 0.0 ) min = 1.0e-06 * (max - min);
    this->UseAbsoluteValue = 0;
    this->LogMinRange = log10((double)min);
    this->LogMaxRange = log10((double)max);
    }
}

// Description:
// Given a scalar value v, return an rgba color value from lookup table. 
// Mapping performed log base 10 (negative ranges are converted into positive
// values).
unsigned char *vtkLogLookupTable::MapValue(float v)
{
  int indx;

  if ( v < this->TableRange[0] ) v = this->TableRange[0];
  else if ( v > this->TableRange[1] ) v = this->TableRange[1];

  if ( this->UseAbsoluteValue )
    {
    indx = (int)( (log10((double)(-v)) - this->LogMinRange) /
                  (this->LogMaxRange - this->LogMinRange) * 
                  (this->NumberOfColors-1) );
    }
  else
    {
    indx = (int)( (log10((double)v) - this->LogMinRange) /
                  (this->LogMaxRange - this->LogMinRange) * 
                  (this->NumberOfColors-1) );
    }

  return this->Table.GetColor(indx);
}

void vtkLogLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLookupTable::PrintSelf(os,indent);

  os << indent << "Log Min Range: " <<this->LogMinRange << "\n";
  os << indent << "Log Max Range: " <<this->LogMaxRange << "\n";
}
