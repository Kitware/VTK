/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellPicker.h
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
// .NAME vtkCellPicker - select a cell by shooting a ray into graphics window
// .SECTION Description
// vtkCellPicker is used to select a cell by shooting a ray into graphics
// window and intersecting with actor's defining geometry - specifically 
// its cells. Beside returning coordinates, actor and mapper, vtkCellPicker
// returns the id of the closest cell within the tolerance along the pick
// ray, and the dataset that was picked.
// .SECTION See Also
// vtkPicker vtkPointPicker

#ifndef __vtkCellPicker_h
#define __vtkCellPicker_h

#include "vtkPicker.h"

class vtkGenericCell;

class VTK_RENDERING_EXPORT vtkCellPicker : public vtkPicker
{
public:
  static vtkCellPicker *New();
  vtkTypeMacro(vtkCellPicker,vtkPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the id of the picked cell. If CellId = -1, nothing was picked.
  vtkGetMacro(CellId, vtkIdType);

  // Description:
  // Get the subId of the picked cell. If SubId = -1, nothing was picked.
  vtkGetMacro(SubId, int);

  // Description:
  // Get the parametric coordinates of the picked cell. Only valid if 
  // pick was made.
  vtkGetVectorMacro(PCoords, float,3);

protected:
  vtkCellPicker();
  ~vtkCellPicker();

  vtkIdType CellId; // picked cell
  int SubId; // picked cell subId
  float PCoords[3]; // picked cell parametric coordinates

  virtual float IntersectWithLine(float p1[3], float p2[3], float tol, 
                                  vtkAssemblyPath *path, vtkProp3D *p, 
                                  vtkAbstractMapper3D *m);
  void Initialize();
  
private:
  vtkGenericCell *Cell; //used to accelerate picking
  
private:
  vtkCellPicker(const vtkCellPicker&);  // Not implemented.
  void operator=(const vtkCellPicker&);  // Not implemented.
};

#endif


