/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGetRemoteGhostCells.h
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
// .NAME vtkGetRemoteGhostCells - get ghost cells from other processes

#ifndef __vtkGetRemoteGhostCells_h
#define __vtkGetRemoteGhostCells_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkMultiProcessController.h"

#define VTK_CELL_ID_TAG      10
#define VTK_POINT_COORDS_TAG 20
#define VTK_NUM_POINTS_TAG   30
#define VTK_NUM_CELLS_TAG    40
#define VTK_POLY_DATA_TAG    50
#define VTK_BOUNDS_TAG       60

class VTK_EXPORT vtkGetRemoteGhostCells : public vtkPolyDataToPolyDataFilter 
{
public:
  vtkTypeMacro(vtkGetRemoteGhostCells, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with LowPoint=(0,0,0) and HighPoint=(0,0,1). Scalar
  // range is (0,1).
  static vtkGetRemoteGhostCells *New();

  // Description:
  // The filter needs a controller to determine which process it is in.
  vtkSetObjectMacro(Controller, vtkMultiProcessController);
  vtkGetObjectMacro(Controller, vtkMultiProcessController); 

protected:
  vtkGetRemoteGhostCells();
  ~vtkGetRemoteGhostCells();
  vtkGetRemoteGhostCells(const vtkGetRemoteGhostCells&) {};
  void operator=(const vtkGetRemoteGhostCells&) {};

  void Execute();

  vtkMultiProcessController *Controller;
  vtkPointLocator *Locator;
  
};

#endif


