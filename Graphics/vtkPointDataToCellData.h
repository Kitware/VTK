/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointDataToCellData.h
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
// .NAME vtkPointDataToCellData - map point data to cell data
// .SECTION Description
// vtkPointDataToCellData is a filter that transforms point data (i.e., data
// specified per point) into cell data (i.e., data specified per cell).
// The method of transformation is based on averaging the data
// values of all points defining a particular cell. Optionally, the input point
// data can be passed through to the output as well.

// .SECTION Caveats
// This filter is an abstract filter, that is, the output is an abstract type
// (i.e., vtkDataSet). Use the convenience methods (e.g.,
// vtkGetPolyDataOutput(), GetStructuredPointsOutput(), etc.) to get the type
// of output you want.

// .SECTION See Also
// vtkDataSetToDataSetFilter vtkPointData vtkCellData vtkCellDataToPointData


#ifndef __vtkPointDataToCellData_h
#define __vtkPointDataToCellData_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkPointDataToCellData : public vtkDataSetToDataSetFilter
{
public:
  static vtkPointDataToCellData *New();
  vtkTypeMacro(vtkPointDataToCellData,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Control whether the input point data is to be passed to the output. If
  // on, then the input point data is passed through to the output; otherwise,
  // only generated point data is placed into the output.
  vtkSetMacro(PassPointData,int);
  vtkGetMacro(PassPointData,int);
  vtkBooleanMacro(PassPointData,int);

protected:
  vtkPointDataToCellData();
  ~vtkPointDataToCellData() {};

  void Execute();

  int PassPointData;
private:
  vtkPointDataToCellData(const vtkPointDataToCellData&);  // Not implemented.
  void operator=(const vtkPointDataToCellData&);  // Not implemented.
};

#endif


