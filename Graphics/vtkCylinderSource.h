/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylinderSource.h
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
// .NAME vtkCylinderSource - generate a cylinder centered at origin
// .SECTION Description
// vtkCylinderSource creates a polygonal cylinder centered at Center;
// The axis of the cylinder is aligned along the global y-axis.
// The height and radius of the cylinder can be specified, as well as the
// number of sides. It is also possible to control whether the cylinder is
// open-ended or capped.

#ifndef __vtkCylinderSource_h
#define __vtkCylinderSource_h

#include "vtkPolyDataSource.h"

class VTK_GRAPHICS_EXPORT vtkCylinderSource : public vtkPolyDataSource 
{
public:
  static vtkCylinderSource *New();
  vtkTypeMacro(vtkCylinderSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the height of the cylinder.
  vtkSetClampMacro(Height,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(Height,float);

  // Description:
  // Set the radius of the cylinder.
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(Radius,float);

  // Description:
  // Set/Get cylinder center
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Set the number of facets used to define cylinder.
  vtkSetClampMacro(Resolution,int,0,VTK_CELL_SIZE)
  vtkGetMacro(Resolution,int);

  // Description:
  // Turn on/off whether to cap cylinder with polygons.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);

protected:
  vtkCylinderSource(int res=6);
  ~vtkCylinderSource() {};
  vtkCylinderSource(const vtkCylinderSource&);
  void operator=(const vtkCylinderSource&);

  void Execute();
  float Height;
  float Radius;
  float Center[3];
  int Resolution;
  int Capping;

};

#endif


