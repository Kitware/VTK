/*=========================================================================

  Program:   Visualization Library
  Module:    PointSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPointSource - create a random cloud of points
// .SECTION Description
// vlPointSource is a source object that creates a user-specified number 
// of points within a specified radius about a specified center point. 
// The location of the points is random within the sphere.

#ifndef __vlPointSource_h
#define __vlPointSource_h

#include "PolySrc.hh"

class vlPointSource : public vlPolySource 
{
public:
  vlPointSource(int numPts=10);
  char *GetClassName() {return "vlPointSource";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set the number of points to generate.
  vlSetClampMacro(NumberOfPoints,int,1,LARGE_INTEGER);
  vlGetMacro(NumberOfPoints,int);

  // Description:
  // Set the center of the point cloud.
  vlSetVector3Macro(Center,float);
  vlGetVectorMacro(Center,float,3);

  // Description:
  // Set the radius of the point cloud.
  vlSetClampMacro(Radius,float,0.0,LARGE_FLOAT);
  vlGetMacro(Radius,float);

protected:
  void Execute();
  int NumberOfPoints;
  float Center[3];
  float Radius;
};

#endif


