/*=========================================================================

  Program:   Visualization Library
  Module:    LineSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlLineSource - create a line defined by two end points
// .SECTION Description
// vlLineSource is a source object that creates a polyline defined by
// two endpoints. The number of segments composing the polyline is
// controlled by setting the object resolution.

#ifndef __vlLineSource_h
#define __vlLineSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_VERTS

class vlLineSource : public vlPolySource 
{
public:
  vlLineSource(int res=1);
  char *GetClassName() {return "vlLineSource";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set position of first end point.
  vlSetVector3Macro(Pt1,float);
  vlGetVectorMacro(Pt1,float,3);

  // Description:
  // Set position of other end point.
  vlSetVector3Macro(Pt2,float);
  vlGetVectorMacro(Pt2,float,3);

  // Description:
  // Divide line into resolution number of pieces.
  vlSetClampMacro(Resolution,int,1,LARGE_INTEGER);
  vlGetMacro(Resolution,int);

protected:
  void Execute();
  float Pt1[3];
  float Pt2[3];
  int Resolution;
};

#endif


