/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlankStructuredGrid.h
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
// .NAME vtkBlankStructuredGrid - translate point attribute data into a blanking field

// .SECTION Description
// vtkBlankStructuredGrid is a filter that sets the blanking field in a 
// vtkStructuredGrid dataset. The blanking field is set by examining a
// specified point attribute data array (e.g., scalars) and converting
// values in the data array to either a "1" (visible) or "0" (blanked) value
// in the blanking array. The values to be blanked are specified by giving
// a min/max range. All data values in the data array indicated and laying
// within the range specified (inclusive on both ends) are translated to 
// a "off" blanking value.

// .SECTION See Also
// vtkStructuredGrid

#ifndef __vtkBlankStructuredGrid_h
#define __vtkBlankStructuredGrid_h

#include "vtkStructuredGridToStructuredGridFilter.h"

class VTK_EXPORT vtkBlankStructuredGrid : public vtkStructuredGridToStructuredGridFilter
{
public:
  static vtkBlankStructuredGrid *New();
  vtkTypeMacro(vtkBlankStructuredGrid,vtkStructuredGridToStructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the lower data value in the data array specified which will be converted
  // into a "blank" (or off) value in the blanking array.
  vtkSetMacro(MinBlankingValue,float);
  vtkGetMacro(MinBlankingValue,float);

  // Description:
  // Specify the upper data value in the data array specified which will be converted
  // into a "blank" (or off) value in the blanking array.
  vtkSetMacro(MaxBlankingValue,float);
  vtkGetMacro(MaxBlankingValue,float);

  // Description:
  // Specify the data array name to use to generate the blanking field. Alternatively,
  // you can specify the array id. (If both are set, the array name takes precedence.)
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);

  // Description:
  // Specify the data array id to use to generate the blanking field. Alternatively,
  // you can specify the array name. (If both are set, the array name 
  // takes precedence.)
  vtkSetMacro(ArrayId,int);
  vtkGetMacro(ArrayId,int);

  // Description:
  // Specify the component in the data array to use to generate the blanking field.
  vtkSetClampMacro(Component,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(Component,int);

protected:
  vtkBlankStructuredGrid();
  ~vtkBlankStructuredGrid();
  vtkBlankStructuredGrid(const vtkBlankStructuredGrid&) {}
  void operator=(const vtkBlankStructuredGrid&) {}

  void Execute();
  
  float MinBlankingValue;
  float MaxBlankingValue;
  char  *ArrayName;
  int   ArrayId;
  int   Component;
  
};

#endif


