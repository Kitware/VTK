/*=========================================================================

  Program:   Visualization Toolkit
  Module:    LineSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkLineSource - create a line defined by two end points
// .SECTION Description
// vtkLineSource is a source object that creates a polyline defined by
// two endpoints. The number of segments composing the polyline is
// controlled by setting the object resolution.

#ifndef __vtkLineSource_h
#define __vtkLineSource_h

#include "PolySrc.hh"

class vtkLineSource : public vtkPolySource 
{
public:
  vtkLineSource(int res=1);
  char *GetClassName() {return "vtkLineSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set position of first end point.
  vtkSetVector3Macro(Pt1,float);
  vtkGetVectorMacro(Pt1,float,3);

  // Description:
  // Set position of other end point.
  vtkSetVector3Macro(Pt2,float);
  vtkGetVectorMacro(Pt2,float,3);

  // Description:
  // Divide line into resolution number of pieces.
  vtkSetClampMacro(Resolution,int,1,LARGE_INTEGER);
  vtkGetMacro(Resolution,int);

protected:
  void Execute();
  float Pt1[3];
  float Pt2[3];
  int Resolution;
};

#endif


