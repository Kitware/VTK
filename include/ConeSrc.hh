/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ConeSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkConeSource - generate polygonal cone 
// .SECTION Description
// vtkConeSource creates a cone centered at origin and pointing down the 
// x-axis. Depending upon the resolution of this object, different 
// representations are created. If resolution=0 a line is created; if 
// resolution=1, a single triangle is created; if resolution=2, two 
// crossed triangles are created. For resolution > 2, a 3D cone (with 
// resolution number of sides) is created. It is also possible to control 
// whether the bottom of the cone is capped with a (resolution-sided) 
// polygon, and to specify the height and radius of the cone.

#ifndef __vtkConeSource_h
#define __vtkConeSource_h

#include "PolySrc.hh"

class vtkConeSource : public vtkPolySource 
{
public:
  vtkConeSource(int res=6);
  char *GetClassName() {return "vtkConeSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the height of the cone.
  vtkSetClampMacro(Height,float,0.0,LARGE_FLOAT)
  vtkGetMacro(Height,float);

  // Description:
  // Set the radius of the cone.
  vtkSetClampMacro(Radius,float,0.0,LARGE_FLOAT)
  vtkGetMacro(Radius,float);

  // Description:
  // Set the number of facets used to represent cone.
  vtkSetClampMacro(Resolution,int,0,MAX_CELL_SIZE)
  vtkGetMacro(Resolution,int);

  // Description:
  // Turn on/off whether to cap cone with polygon.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);

protected:
  void Execute();
  float Height;
  float Radius;
  int Resolution;
  int Capping;

};

#endif


