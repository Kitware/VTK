/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CylSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCylinderSource - generate a cylinder centered at origin
// .SECTION Description
// vtkCylinderSource creates a polygonal cylinder centered at the origin.
// The axis of the cylinder is aligned along the global y-axis.
// The height and radius of the cylinder can be specified, as well as the
// number of sides. It is also possible to control whether the cylinder is
// open-ended or capped.

#ifndef __vtkCylinderSource_h
#define __vtkCylinderSource_h

#include "PolySrc.hh"

class vtkCylinderSource : public vtkPolySource 
{
public:
  vtkCylinderSource(int res=6);
  char *GetClassName() {return "vtkCylinderSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the height of the cylinder.
  vtkSetClampMacro(Height,float,0.0,LARGE_FLOAT)
  vtkGetMacro(Height,float);

  // Description:
  // Set the radius of the cylinder.
  vtkSetClampMacro(Radius,float,0.0,LARGE_FLOAT)
  vtkGetMacro(Radius,float);

  // Description:
  // Set the number of facets used to define cylinder.
  vtkSetClampMacro(Resolution,int,0,MAX_CELL_SIZE)
  vtkGetMacro(Resolution,int);

  // Description:
  // Turn on/off whether to cap cylinder with polygons.
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


