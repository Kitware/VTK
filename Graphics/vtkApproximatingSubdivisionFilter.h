/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkApproximatingSubdivisionFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    This work was supported bt PHS Research Grant No. 1 P41 RR13218-01
             from the National Center for Research Resources


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
// .NAME vtkApproximatingSubdivisionFilter - generate a subdivision surface using an Approximating Scheme
// .SECTION Description
// vtkApproximatingSubdivisionFilter is an abstract class that defines
// the protocol for Approximating subdivision surface filters.

#ifndef __vtkApproximatingSubdivisionFilter_h
#define __vtkApproximatingSubdivisionFilter_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"
#include "vtkCellArray.h"

class VTK_GRAPHICS_EXPORT vtkApproximatingSubdivisionFilter : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeMacro(vtkApproximatingSubdivisionFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the number of subdivisions.
  vtkSetMacro(NumberOfSubdivisions,int);
  vtkGetMacro(NumberOfSubdivisions,int);

protected:
  vtkApproximatingSubdivisionFilter();
  ~vtkApproximatingSubdivisionFilter() {};

  void Execute();
  virtual void GenerateSubdivisionPoints (vtkPolyData *inputDS,
                                          vtkIntArray *edgeData,
                                          vtkPoints *outputPts,
                                          vtkPointData *outputPD) = 0;
  void GenerateSubdivisionCells (vtkPolyData *inputDS, vtkIntArray *edgeData,
                                 vtkCellArray *outputPolys,
                                 vtkCellData *outputCD);
  int FindEdge (vtkPolyData *mesh, vtkIdType cellId, vtkIdType p1,
                vtkIdType p2, vtkIntArray *edgeData, vtkIdList *cellIds);
  vtkIdType InterpolatePosition (vtkPoints *inputPts, vtkPoints *outputPts,
                                 vtkIdList *stencil, float *weights);
  int NumberOfSubdivisions;
private:
  vtkApproximatingSubdivisionFilter(const vtkApproximatingSubdivisionFilter&);  // Not implemented.
  void operator=(const vtkApproximatingSubdivisionFilter&);  // Not implemented.
};

#endif


