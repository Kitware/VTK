/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConeSource.h
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
// .NAME vtkConeSource - generate polygonal cone 
// .SECTION Description
// vtkConeSource creates a cone centered at origin and pointing down the 
// x-axis. Depending upon the resolution of this object, different 
// representations are created. If resolution=0 a line is created; if 
// resolution=1, a single triangle is created; if resolution=2, two 
// crossed triangles are created. For resolution > 2, a 3D cone (with 
// resolution number of sides) is created. It also is possible to control 
// whether the bottom of the cone is capped with a (resolution-sided) 
// polygon, and to specify the height and radius of the cone.

#ifndef __vtkConeSource_h
#define __vtkConeSource_h

#include "vtkPolyDataSource.h"

class VTK_GRAPHICS_EXPORT vtkConeSource : public vtkPolyDataSource 
{
public:
  vtkTypeMacro(vtkConeSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with default resolution 6, height 1.0, radius 0.5, and
  // capping on.
  static vtkConeSource *New();

  // Description:
  // Set the height of the cone.
  vtkSetClampMacro(Height,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(Height,float);

  // Description:
  // Set the radius of the cone.
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(Radius,float);

  // Description:
  // Set the number of facets used to represent the cone.
  vtkSetClampMacro(Resolution,int,0,VTK_CELL_SIZE)
  vtkGetMacro(Resolution,int);

  // Description:
  // Set the angle of the cone. As a side effect, this method sets radius.
  void SetAngle (float angle);
  float GetAngle ();

  // Description:
  // Turn on/off whether to cap the cone with a polygon.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);

protected:
  vtkConeSource(int res=6);
  ~vtkConeSource() {};

  void Execute();
  void ExecuteInformation();
  float Height;
  float Radius;
  int Resolution;
  int Capping;
private:
  vtkConeSource(const vtkConeSource&);  // Not implemented.
  void operator=(const vtkConeSource&);  // Not implemented.
};

#endif


