/*=========================================================================

  Program:   Visualization Library
  Module:    SpherSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Create sphere centered at origin. Resolution=0 means octahedron which 
// is recursively subdivided for each resolution increase.
//
#ifndef __vlSphereSource_h
#define __vlSphereSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION 8

class vlSphereSource : public vlPolySource 
{
public:
  vlSphereSource(int res=2);
  char *GetClassName() {return "vlSphereSource";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(Radius,float,0.0,LARGE_FLOAT)
  vlGetMacro(Radius,float);

  vlSetClampMacro(Resolution,int,0,MAX_RESOLUTION)
  vlGetMacro(Resolution,int);

protected:
  void Execute();
  float Radius;
  int Resolution;

};

#endif


