/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLogLookupTable.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>
#include "vtkLogLookupTable.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkLogLookupTable* vtkLogLookupTable::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLogLookupTable");
  if(ret)
    {
    return (vtkLogLookupTable*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLogLookupTable;
}




// Construct with (minimum,maximum) range 1 to 10 (based on 
// logarithmic values).
vtkLogLookupTable::vtkLogLookupTable(int sze, int ext):
vtkLookupTable(sze,ext)
{
  this->LogMinRange = 0.0;
  this->LogMaxRange = 1.0;
  this->UseAbsoluteValue = 0;
}

// Set the minimum/maximum scalar values for scalar mapping. Scalar values
// less than minimum range value are clamped to minimum range value.
// Scalar values greater than maximum range value are clamped to maximum
// range value. (The log base 10 of these values is taken and mapping is
// performed in logarithmic space.)
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
    if ( max == 0.0 )
      {
      max = 1.0e-06 * (min - max);
      }
    this->UseAbsoluteValue = 1;
    this->LogMinRange = log10((double)(-min));
    this->LogMaxRange = log10((double)(-max));
    }
  else 
    {
    if ( min == 0.0 )
      {
      min = 1.0e-06 * (max - min);
      }
    this->UseAbsoluteValue = 0;
    this->LogMinRange = log10((double)min);
    this->LogMaxRange = log10((double)max);
    }
}

// Given a scalar value v, return an rgba color value from lookup table. 
// Mapping performed log base 10 (negative ranges are converted into positive
// values).
unsigned char *vtkLogLookupTable::MapValue(float v)
{
  int indx;

  if ( v < this->TableRange[0] )
    {
    v = this->TableRange[0];
    }
  else if ( v > this->TableRange[1] )
    {
    v = this->TableRange[1];
    }

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

  return this->Table->GetPointer(4*indx);
}

template <class T>
static void vtkLogLookupTableMapData(vtkLogLookupTable *self,
				    T *input, unsigned char *output,
				    int i, int inIncr, int outIncr) 
{
  unsigned char *cptr;
  int j;

  if (outIncr != 2)
    {
    while (--i >= 0) 
      {
      cptr = self->MapValue(*input);
      for (j = 0; j < outIncr; j++)
	{
        *output++ = *cptr++;
	}
      input += inIncr;
      }
    }
  else
    {
    while (--i >= 0) 
      {
      cptr = self->MapValue(*input);
      *output++ = cptr[0];
      *output++ = cptr[3];
      }
    }
}

void vtkLogLookupTable::MapScalarsThroughTable2(void *input, 
						unsigned char *output,
						int inputDataType, 
						int numberOfValues,
						int inputIncrement,
						int outputFormat)
{
  switch (inputDataType)
    {
    case VTK_CHAR:
      vtkLogLookupTableMapData(this,(char *)input,output,numberOfValues,inputIncrement,outputFormat);
      break;
      
    case VTK_UNSIGNED_CHAR:
      vtkLogLookupTableMapData(this,(unsigned char *)input,output,numberOfValues,inputIncrement,outputFormat);
      break;
      
    case VTK_SHORT:
      vtkLogLookupTableMapData(this,(short *)input,output,numberOfValues,inputIncrement,outputFormat);
      break;
      
    case VTK_UNSIGNED_SHORT:
      vtkLogLookupTableMapData(this,(unsigned short *)input,output,numberOfValues,inputIncrement,outputFormat);
      break;
      
    case VTK_INT:
      vtkLogLookupTableMapData(this,(int *)input,output,numberOfValues,inputIncrement,outputFormat);
      break;
      
    case VTK_UNSIGNED_INT:
      vtkLogLookupTableMapData(this,(unsigned int *)input,output,numberOfValues,inputIncrement,outputFormat);
      break;
      
    case VTK_LONG:
      vtkLogLookupTableMapData(this,(long *)input,output,numberOfValues,inputIncrement,outputFormat);
      break;
      
    case VTK_UNSIGNED_LONG:
      vtkLogLookupTableMapData(this,(unsigned long *)input,output,numberOfValues,inputIncrement,outputFormat);
      break;
      
    case VTK_FLOAT:
      vtkLogLookupTableMapData(this,(float *)input,output,numberOfValues,inputIncrement,outputFormat);
      break;
      
    case VTK_DOUBLE:      vtkLogLookupTableMapData(this,(double *)input,output,numberOfValues,inputIncrement,outputFormat);
      break;
      
    default:
      vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
      return;
    }
}  

void vtkLogLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLookupTable::PrintSelf(os,indent);

  os << indent << "Log Min Range: " <<this->LogMinRange << "\n";
  os << indent << "Log Max Range: " <<this->LogMaxRange << "\n";
}
