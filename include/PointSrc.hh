/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PointSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPointSource - create a random cloud of points
// .SECTION Description
// vtkPointSource is a source object that creates a user-specified number 
// of points within a specified radius about a specified center point. 
// The location of the points is random within the sphere.

#ifndef __vtkPointSource_h
#define __vtkPointSource_h

#include "PolySrc.hh"

class vtkPointSource : public vtkPolySource 
{
public:
  vtkPointSource(int numPts=10);
  ~vtkPointSource() {};
  char *GetClassName() {return "vtkPointSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of points to generate.
  vtkSetClampMacro(NumberOfPoints,int,1,LARGE_INTEGER);
  vtkGetMacro(NumberOfPoints,int);

  // Description:
  // Set the center of the point cloud.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Set the radius of the point cloud.
  vtkSetClampMacro(Radius,float,0.0,LARGE_FLOAT);
  vtkGetMacro(Radius,float);

protected:
  void Execute();
  int NumberOfPoints;
  float Center[3];
  float Radius;
};

#endif


