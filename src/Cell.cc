/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Cell.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Cell.hh"
//
// Instantiate cell from outside
//
void vtkCell::Initialize(int npts, int *pts, vtkPoints *p)
{
  for (int i=0; i<npts; i++)
    {
    this->PointIds.InsertId(i,pts[i]);
    this->Points.SetPoint(i,p->GetPoint(pts[i]));
    }
}
 
// Description:
// Bounding box intersection modified from Graphics Gems Vol I.
// Note: the intersection ray is assumed normalized such that
// valid intersections can only occur between [0,1]. Method returns non-zero
// value if bounding box is hit. Origin[3] starts the ray, dir[3] is the 
// components of the ray in the x-y-z directions, coord[3] is the location 
// of hit, and t is the parametric coordinate along line.
#define RIGHT 0
#define LEFT 1
#define MIDDLE 2

char vtkCell::HitBBox (float bounds[6], float origin[3], float dir[3], 
                      float coord[3], float& t)
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
  else
    t = maxT[whichPlane];
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

// Description:
// Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Return pointer
// to array of six float values.
float *vtkCell::GetBounds ()
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

// Description:
// Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Copy result into
// user provided array.
void vtkCell::GetBounds(float bounds[6])
{
  float *b=this->GetBounds();
  for (int i=0; i < 6; i++) bounds[i] = b[i];
}

// Description:
// Compute Length squared of cell (i.e., bounding box diagonal squared)
float vtkCell::GetLength2 ()
{
  float diff, l=0.0;
  float *bounds;
  int i;

  bounds = this->GetBounds();

  for (i=0; i<3; i++)
    {
    diff = bounds[2*i+1] - bounds[2*i];
    l += diff * diff;
    }
 
  return l;
}

void vtkCell::PrintSelf(ostream& os, vtkIndent indent)
{
  float *bounds;
  int i;

  vtkObject::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->PointIds.GetNumberOfIds() << "\n";
  bounds = this->GetBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";

  os << indent << "  Point ids are: ";
  for (i=0; this->PointIds.GetNumberOfIds(); i++)
    {
    os << ", " << this->PointIds.GetId(i);
    if ( i && !(i % 12) ) os << "\n\t";
    }
  os << indent << "\n";
}
