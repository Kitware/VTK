/*=========================================================================

  Program:   Visualization Library
  Module:    Polygon.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Computational class for polygons.
//
#ifndef __vlPolygon_h
#define __vlPolygon_h

#include "Cell.hh"
#include "Points.hh"

#define MAX_RESOLUTION MAX_VERTS

class vlPolygon : public vlCell
{
public:
  vlPolygon() {};
  char *GetClassName() {return "vlPolygon";};

  void ComputeNormal(vlPoints *p, int numPts, int *pts, float *n);
  void ComputeNormal(float *v1, float *v2, float *v3, float *n);

  float DistanceToPoint(float *x);
};

#endif


