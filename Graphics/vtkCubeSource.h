/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeSource.h
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
// .NAME vtkCubeSource - create a polygonal representation of a cube
// .SECTION Description
// vtkCubeSource creates a cube centered at origin. The cube is represented
// with four-sided polygons. It is possible to specify the length, width, 
// and height of the cube independently.

#ifndef __vtkCubeSource_h
#define __vtkCubeSource_h

#include "vtkPolyDataSource.h"

class VTK_GRAPHICS_EXPORT vtkCubeSource : public vtkPolyDataSource 
{
public:
  static vtkCubeSource *New();
  vtkTypeMacro(vtkCubeSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the length of the cube in the x-direction.
  vtkSetClampMacro(XLength,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(XLength,float);

  // Description:
  // Set the length of the cube in the y-direction.
  vtkSetClampMacro(YLength,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(YLength,float);

  // Description:
  // Set the length of the cube in the z-direction.
  vtkSetClampMacro(ZLength,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(ZLength,float);

  // Description:
  // Set the center of the cube.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Convenience method allows creation of cube by specifying bounding box.
  void SetBounds(float xMin, float xMax,
	         float yMin, float yMax,
		 float zMin, float zMax);
  void SetBounds(float bounds[6]);


protected:
  vtkCubeSource(float xL=1.0, float yL=1.0, float zL=1.0);
  ~vtkCubeSource() {};
  vtkCubeSource(const vtkCubeSource&);
  void operator=(const vtkCubeSource&);

  void Execute();
  float XLength;
  float YLength;
  float ZLength;
  float Center[3];
};

#endif


