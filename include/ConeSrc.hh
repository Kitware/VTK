/*=========================================================================

  Program:   Visualization Library
  Module:    ConeSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlConeSource - generate polygonal cone 
// .SECTION Description
// vlConeSource creates a cone centered at origin. Depending upon the
// resolution of this object, different representations are created.
// If resolution=0 a line is created; if resolution=1, a single
// triangle is created; if resolution=2, two crossed triangles are
// created. For resolution > 2, a 3D cone (with resolution sides) is
// created. It is also possible to control whether the bottom of the
// cone is capped with a (resolution-sided) polygon, and to specify 
// the height and radius of the cone.

#ifndef __vlConeSource_h
#define __vlConeSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_CELL_SIZE

class vlConeSource : public vlPolySource 
{
public:
  vlConeSource(int res=6);
  char *GetClassName() {return "vlConeSource";};
  void PrintSelf(ostream& os, vlIndent indent);

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
  void Execute();
  float Height;
  float Radius;
  int Resolution;
  int Capping;

};

#endif


