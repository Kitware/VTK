/*=========================================================================

  Program:   Visualization Library
  Module:    PlaneSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME - create an array of quadrilaterals located in the plane
// .SECTION Description
// vlPlaneSource creates an m x n array of quadrilaterals arranged as a
// regular tiling in the plane. The plane is centered at the origin, and 
// orthogonal to the global z-axis.  The resolution of the plane can be
// specified in both the x and y directions (i.e., specify m and n, 
// respectively).

#ifndef __vlPlaneSource_h
#define __vlPlaneSource_h

#include "PolySrc.hh"

class vlPlaneSource : public vlPolySource 
{
public:
  vlPlaneSource() : XRes(1), YRes(1) {};
  vlPlaneSource(const int xR, const int yR) {XRes=xR; YRes=yR;};
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlPlaneSource";};

  void SetResolution(const int xR, const int yR);
  void GetResolution(int& xR,int& yR) {xR=this->XRes; yR=this->YRes;};

protected:
  void Execute();
  int XRes;
  int YRes;
};

#endif


