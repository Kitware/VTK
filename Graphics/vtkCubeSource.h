/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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
  vtkTypeRevisionMacro(vtkCubeSource,vtkPolyDataSource);
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

  void Execute();
  float XLength;
  float YLength;
  float ZLength;
  float Center[3];
private:
  vtkCubeSource(const vtkCubeSource&);  // Not implemented.
  void operator=(const vtkCubeSource&);  // Not implemented.
};

#endif


