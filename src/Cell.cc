/*=========================================================================

  Program:   Visualization Library
  Module:    Cell.cc
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
#include "Cell.hh"
//
// Instantiate cell from outside
//
void vlCell::Initialize(int npts, int *pts, vlPoints *p)
{
  for (int i=0; i<npts; i++)
    {
    this->PointIds.InsertId(i,pts[i]);
    this->Points.SetPoint(i,p->GetPoint(pts[i]));
    }
}
 
//
//  Bounding box intersection modified from Graphics Gems Vol I.
//  Note: the intersection ray is assumed normalized such that
//  valid intersections can only occur between [0,1].
//
#define RIGHT 0
#define LEFT 1
#define MIDDLE 2

char vlCell::HitBBox (float bounds[6], float origin[3], float dir[3], float coord[3])
{
  char    inside=1;
  char    quadrant[3];
  int     i, whichPlane=0;
  float   maxT[3], candidatePlane[3];
//
//  First find closest planes
//
  for (i=0; i<3; i++) 
    {
    if ( origin[i] < bounds[2*i] ) 
      {
      quadrant[i] = LEFT;
      candidatePlane[i] = bounds[2*i];
      inside = 0;
      }
    else if ( origin[i] > bounds[2*i+1] ) 
      {
      quadrant[i] = RIGHT;
      candidatePlane[i] = bounds[2*i+1];
      inside = 0;
      }
    else 
      {
      quadrant[i] = MIDDLE;
      }
    }
//
//  Check whether origin of ray is inside bbox
//
  if (inside) return 1;
//
//  Calculate parametric distances to plane
//
  for (i=0; i<3; i++)
    {
    if ( quadrant[i] != MIDDLE && dir[i] != 0.0 )
      {
      maxT[i] = (candidatePlane[i]-origin[i]) / dir[i];
      }
    else
      {
      maxT[i] = -1.0;
      }
    }
//
//  Find the largest parametric value of intersection
//
  for (i=0; i<3; i++)
    if ( maxT[whichPlane] < maxT[i] )
      whichPlane = i;
//
//  Check for valid intersection along line
//
  if ( maxT[whichPlane] > 1.0 || maxT[whichPlane] < 0.0 )
    return 0;
//
//  Intersection point along line is okay.  Check bbox.
//
  for (i=0; i<3; i++) 
    {
    if (whichPlane != i) 
      {
      coord[i] = origin[i] + maxT[whichPlane]*dir[i];
      if ( coord[i] < bounds[2*i] || coord[i] > bounds[2*i+1] )
        return 0;
      } 
    else 
      {
      coord[i] = candidatePlane[i];
      }
    }

    return 1;
}

float *vlCell::GetBounds ()
{
  float *x;
  int i, j;
  static float bounds[6];

  bounds[0] = bounds[2] = bounds[4] =  LARGE_FLOAT;
  bounds[1] = bounds[3] = bounds[5] = -LARGE_FLOAT;

  for (i=0; i<this->Points.GetNumberOfPoints(); i++)
    {
    x = this->Points.GetPoint(i);
    for (j=0; j<3; j++)
      {
      if ( x[j] < bounds[2*j] ) bounds[2*j] = x[j];
      if ( x[j] > bounds[2*j+1] ) bounds[2*j+1] = x[j];
      }
    }
  return bounds;
}
