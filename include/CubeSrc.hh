/*=========================================================================

  Program:   Visualization Library
  Module:    CubeSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCubeSource - create a polygonal representation of a cube
// .SECTION Description
// vlCubeSource creates a cube centered at origin. The cube is represented
// with four-sided polygons. It is possible to specify the length, width, 
// and height of the cube independently.

#ifndef __vlCubeSource_h
#define __vlCubeSource_h

#include "PolySrc.hh"

class vlCubeSource : public vlPolySource 
{
public:
  vlCubeSource(float xL=1.0, float yL=1.0, float zL=1.0);
  char *GetClassName() {return "vlCubeSource";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set the length of the cube in the x-direction.
  vlSetClampMacro(XLength,float,0.0,LARGE_FLOAT);
  vlGetMacro(XLength,float);

  // Description:
  // Set the length of the cube in the y-direction.
  vlSetClampMacro(YLength,float,0.0,LARGE_FLOAT);
  vlGetMacro(YLength,float);

  // Description:
  // Set the length of the cube in the z-direction.
  vlSetClampMacro(ZLength,float,0.0,LARGE_FLOAT);
  vlGetMacro(ZLength,float);

  // Description:
  // Set the center of the cube.
  vlSetVector3Macro(Center,float);
  vlGetVectorMacro(Center,float,3);

  void SetBounds(float bounds[6]);

protected:
  void Execute();
  float XLength;
  float YLength;
  float ZLength;
  float Center[3];
};

#endif


