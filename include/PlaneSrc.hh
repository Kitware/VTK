/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PlaneSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPlaneSource - create an array of quadrilaterals located in the plane
// .SECTION Description
// vtkPlaneSource creates an m x n array of quadrilaterals arranged as a
// regular tiling in the plane. The plane is centered at the origin, and 
// orthogonal to the global z-axis.  The resolution of the plane can be
// specified in both the x and y directions (i.e., specify m and n, 
// respectively).

#ifndef __vtkPlaneSource_h
#define __vtkPlaneSource_h

#include "PolySrc.hh"

class vtkPlaneSource : public vtkPolySource 
{
public:
  vtkPlaneSource() : XRes(1), YRes(1) {};
  vtkPlaneSource(const int xR, const int yR) {XRes=xR; YRes=yR;};
  void PrintSelf(ostream& os, vtkIndent indent);
  char *GetClassName() {return "vtkPlaneSource";};

  void SetResolution(const int xR, const int yR);
  void GetResolution(int& xR,int& yR) {xR=this->XRes; yR=this->YRes;};

protected:
  void Execute();
  int XRes;
  int YRes;
};

#endif


