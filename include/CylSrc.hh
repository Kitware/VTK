/*=========================================================================

  Program:   Visualization Library
  Module:    CylSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Created cylinder centered at origin
//
#ifndef __vlCylinderSource_h
#define __vlCylinderSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_VERTS

class vlCylinderSource : public vlPolySource 
{
public:
  vlCylinderSource(int res=6);
  char *GetClassName() {return "vlCylinderSource";};
  void Execute();

  vlSetClampMacro(Height,float,0.0,LARGE_FLOAT)
  vlGetMacro(Height,float);

  vlSetClampMacro(Radius,float,0.0,LARGE_FLOAT)
  vlGetMacro(Radius,float);

  vlSetClampMacro(Resolution,int,0,MAX_RESOLUTION)
  vlGetMacro(Resolution,int);

  vlSetMacro(Capping,int);
  vlGetMacro(Capping,int);
  vlBooleanMacro(Capping,int);

protected:
  float Height;
  float Radius;
  int Resolution;
  int Capping;

};

#endif


