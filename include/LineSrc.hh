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
//
// Create line centered at origin
//
#ifndef __vlLineSource_h
#define __vlLineSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_VERTS

class vlLineSource : public vlPolySource 
{
public:
  vlLineSource(int res=1);
  char *GetClassName() {return "vlLineSource";};

  vlSetVector3Macro(Pt1,float);
  vlGetVectorMacro(Pt1,float);

  vlSetVector3Macro(Pt2,float);
  vlGetVectorMacro(Pt2,float);

  vlSetClampMacro(Resolution,int,1,LARGE_INTEGER);
  vlGetMacro(Resolution,int);

protected:
  void Execute();
  float Pt1[3];
  float Pt2[3];
  int Resolution;
};

#endif


