/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredData.h
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
// .NAME vtkStructuredData - abstract class for topologically regular data
// .SECTION Description
// vtkStructuredData is an abstract class that specifies an interface for
// topologically regular data. Regular data is data that can be accessed
// in rectangular fashion using an i-j-k index. A finite difference grid,
// a volume, or a pixmap are all considered regular.

#ifndef __vtkStructuredData_h
#define __vtkStructuredData_h

#include "vtkObject.h"
#include "vtkIdList.h"

#define VTK_UNCHANGED 0
#define VTK_SINGLE_POINT 1
#define VTK_X_LINE 2
#define VTK_Y_LINE 3
#define VTK_Z_LINE 4
#define VTK_XY_PLANE 5
#define VTK_YZ_PLANE 6
#define VTK_XZ_PLANE 7
#define VTK_XYZ_GRID 8

class VTK_EXPORT vtkStructuredData : public vtkObject 
{
public:
  static vtkStructuredData *New();
  vtkTypeMacro(vtkStructuredData,vtkObject);

  // Description:
  // Specify the dimensions of a regular, rectangular dataset. The input is
  // the new dimensions (inDim) and the current dimensions (dim). The function 
  // returns the dimension of the dataset (0-3D). If the dimensions are 
  // improperly specified a -1 is returned. If the dimensions are unchanged, a
  // value of 100 is returned.
  static int SetDimensions(int inDim[3], int dim[3]);
  static int SetExtent(int inExt[3], int ext[3]);

  // Description:
  // Return the topological dimension of the data (e.g., 0, 1, 2, or 3D).
  static int GetDataDimension(int dataDescription);

  // Description:
  // Get the points defining a cell. (See vtkDataSet for more info.)
  static void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds, 
                            int dataDescription, int dim[3]);

  // Description:
  // Get the cells using a point. (See vtkDataSet for more info.)
  static void GetPointCells(vtkIdType ptId, vtkIdList *cellIds, int dim[3]);

  // Description:
  // Get the cells using the points ptIds, exclusive of the cell cellId. 
  // (See vtkDataSet for more info.)
  static void GetCellNeigbors(vtkIdType cellId, vtkIdList *ptIds, 
                              vtkIdList *cellIds, int dim[3]);

  // Description:
  // Given a location in structured coordinates (i-j-k), and the dimensions
  // of the structured dataset, return the point id.
  static vtkIdType ComputePointId(int dim[3], int ijk[3]) {
    return ijk[2]*dim[0]*dim[1] + ijk[1]*dim[0] + ijk[0];}

  // Description:
  // Given a location in structured coordinates (i-j-k), and the dimensions
  // of the structured dataset, return the cell id.
  static vtkIdType ComputeCellId(int dim[3], int ijk[3]) {
    return ijk[2]*(dim[0]-1)*(dim[1]-1) + ijk[1]*(dim[0]-1) + ijk[0];};

protected:
  vtkStructuredData() {};
  ~vtkStructuredData() {};
  vtkStructuredData(const vtkStructuredData&) {};
  void operator=(const vtkStructuredData&) {};
  
};


#endif

