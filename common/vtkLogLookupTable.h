/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLogLookupTable.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkLogLookupTable - map scalar values into colors using logarithmic (base 10) color table
// .SECTION Description
// vtkLogLookupTable is an object that is used by mapper objects to map scalar 
// values into rgba (red-green-blue-alpha transparency) color specification, 
// or rgba into scalar values. The difference between this class and its
// superclass vtkLookupTable is that this class performs scalar mapping based
// on a logarithmic lookup process. (Uses log base 10.)
//
// If non-positive ranges are encountered, then they are converted to 
// positive values using absolute value.
//
// .SECTION See Also
// vtkLookupTable

#ifndef __vtkLogLookupTable_h
#define __vtkLogLookupTable_h

#include "vtkLookupTable.h"

class VTK_EXPORT vtkLogLookupTable : public vtkLookupTable
{
public:
  static vtkLogLookupTable *New();

  vtkTypeMacro(vtkLogLookupTable,vtkLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the minimum/maximum scalar values for scalar mapping. Scalar values
  // less than minimum range value are clamped to minimum range value.
  // Scalar values greater than maximum range value are clamped to maximum
  // range value. (The log base 10 of these values is taken and mapping is
  // performed in logarithmic space.)
  void SetTableRange(float min, float max);
  void SetTableRange(float r[2]) { this->SetTableRange(r[0], r[1]);}; 

  // Description:
  // Given a scalar value v, return an rgba color value from lookup table. 
  // Mapping performed log base 10 (negative ranges are converted into positive
  // values).
  unsigned char *MapValue(float v);

  // Description:
  // map a set of scalars through the lookup table
  void MapScalarsThroughTable2(void *input, unsigned char *output,
			      int inputDataType, int numberOfValues,
			      int inputIncrement, int outputIncrement);
protected:
  vtkLogLookupTable(int sze=256, int ext=256);
  ~vtkLogLookupTable() {};
  vtkLogLookupTable(const vtkLogLookupTable&) {};
  void operator=(const vtkLogLookupTable&) {};

  float LogMinRange;
  float LogMaxRange;
  float UseAbsoluteValue;
};

#endif
