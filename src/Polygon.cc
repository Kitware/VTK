/*=========================================================================

  Program:   Visualization Library
  Module:    Polygon.cc
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
#include "Polygon.hh"
#include "vlMath.hh"

void vlPolygon::ComputeNormal(vlPoints *p, int numPts, int *pts, float *n)
{
  int     i;
  float   *v1, *v2, *v3;
  float    length;
  float    ax, ay, az;
  float    bx, by, bz;
//
//  Because some polygon vertices are colinear, need to make sure
//  first non-zero normal is found.
//
  v1 = p->GetPoint(pts[0]);
  v2 = p->GetPoint(pts[1]);
  v3 = p->GetPoint(pts[2]);

  for (i=0; i<numPts; i++) 
    {
    ax = v2[0] - v1[0]; ay = v2[1] - v1[1]; az = v2[2] - v1[2];
    bx = v3[0] - v1[0]; by = v3[1] - v1[1]; bz = v3[2] - v1[2];
    n[0] = (ay * bz - az * by);
    n[1] = (az * bx - ax * bz);
    n[2] = (ax * by - ay * bx);

    length = sqrt (n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (length != 0.0) 
      {
      n[0] /= length;
      n[1] /= length;
      n[2] /= length;
      return;
      } 
    else 
      {
      v1 = v2;
      v2 = v3;
      v3 = p->GetPoint(pts[(i+3)%numPts]);
      }
    }
}

void vlPolygon::ComputeNormal(float *v1, float *v2, float *v3, float *n)
{
    float    length;
    float    ax, ay, az;
    float    bx, by, bz;

    // order is important!!! to maintain consistency with polygon vertex order 
    ax = v3[0] - v2[0]; ay = v3[1] - v2[1]; az = v3[2] - v2[2];
    bx = v1[0] - v2[0]; by = v1[1] - v2[1]; bz = v1[2] - v2[2];

    n[0] = (ay * bz - az * by);
    n[1] = (az * bx - ax * bz);
    n[2] = (ax * by - ay * bx);

    length = sqrt (n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (length != 0.0) {
        n[0] /= length;
        n[1] /= length;
        n[2] /= length;
    }
}
