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
// .NAME vlCylinderSource - generate a cylinder centered at origin
// .SECTION Description
// vlCylinderSource creates a polygonal cylinder centered at the origin.
// The axis of the cylinder is aligned along the global y-axis.
// The height and radius of the cylinder can be specified, as well as the
// number of sides. It is also possible to control whether the cylinder is
// open-ended or capped.

#ifndef __vlCylinderSource_h
#define __vlCylinderSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_CELL_SIZE

class vlCylinderSource : public vlPolySource 
{
public:
  vlCylinderSource(int res=6);
  char *GetClassName() {return "vlCylinderSource";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set the height of the cylinder.
  vlSetClampMacro(Height,float,0.0,LARGE_FLOAT)
  vlGetMacro(Height,float);

  // Description:
  // Set the radius of the cylinder.
  vlSetClampMacro(Radius,float,0.0,LARGE_FLOAT)
  vlGetMacro(Radius,float);

  // Description:
  // Set the number of facets used to define cylinder.
  vlSetClampMacro(Resolution,int,0,MAX_RESOLUTION)
  vlGetMacro(Resolution,int);

  // Description:
  // Turn on/off whether to cap cylinder with polygons.
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


