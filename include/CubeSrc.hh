/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CubeSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCubeSource - create a polygonal representation of a cube
// .SECTION Description
// vtkCubeSource creates a cube centered at origin. The cube is represented
// with four-sided polygons. It is possible to specify the length, width, 
// and height of the cube independently.

#ifndef __vtkCubeSource_h
#define __vtkCubeSource_h

#include "PolySrc.hh"

class vtkCubeSource : public vtkPolySource 
{
public:
  vtkCubeSource(float xL=1.0, float yL=1.0, float zL=1.0);
  char *GetClassName() {return "vtkCubeSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the length of the cube in the x-direction.
  vtkSetClampMacro(XLength,float,0.0,LARGE_FLOAT);
  vtkGetMacro(XLength,float);

  // Description:
  // Set the length of the cube in the y-direction.
  vtkSetClampMacro(YLength,float,0.0,LARGE_FLOAT);
  vtkGetMacro(YLength,float);

  // Description:
  // Set the length of the cube in the z-direction.
  vtkSetClampMacro(ZLength,float,0.0,LARGE_FLOAT);
  vtkGetMacro(ZLength,float);

  // Description:
  // Set the center of the cube.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  void SetBounds(float bounds[6]);

protected:
  void Execute();
  float XLength;
  float YLength;
  float ZLength;
  float Center[3];
};

#endif


