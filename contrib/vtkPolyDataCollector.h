/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataCollector.h
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
// .NAME vtkPolyDataCollector - appends one or more polygonal datasets together
// .SECTION Description
// vtkPolyDataCollector is a filter that appends one of more polygonal datasets
// into a single polygonal dataset. All geometry is extracted and appended, 
// but point attributes (i.e., scalars, vectors, normals) are extracted 
// and appended only if all datasets have the point attributes available.
// (For example, if one dataset has scalars but another does not, scalars 
// will not be appended.)

// .SECTION See Also
// vtkAppendFilter

#ifndef __vtkPolyDataCollector_h
#define __vtkPolyDataCollector_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkAppendPolyData.h"

class VTK_EXPORT vtkPolyDataCollector : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkPolyDataCollector *New();

  vtkTypeMacro(vtkPolyDataCollector,vtkPolyDataToPolyDataFilter);
  void PrintSelf(vtkOstream& os, vtkIndent indent);

  void SetInputMemoryLimit(unsigned long limit);
  
protected:
  vtkPolyDataCollector();
  ~vtkPolyDataCollector();
  vtkPolyDataCollector(const vtkPolyDataCollector&) {};
  void operator=(const vtkPolyDataCollector&) {};
  
  // initialize append
  void StreamExecuteStart();
  // clean up after appends.
  void StreamExecuteEnd();
  // append the pieces
  void Execute();
  
  int GetNumberOfStreamDivisions();

  virtual int ComputeDivisionExtents(vtkDataObject *output, 
				     int division, int numDivisions);


  vtkAppendPolyData *AppendFilter;
};

#endif





