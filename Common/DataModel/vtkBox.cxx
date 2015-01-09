/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBox.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkBoundingBox.h"
#include <cassert>

vtkStandardNewMacro(vtkBox);

// Construct the box centered at the origin and each side length 1.0.
//----------------------------------------------------------------------------
vtkBox::vtkBox()
{
  this->BBox = new vtkBoundingBox;
}

//----------------------------------------------------------------------------
// Destroy the bounding box
vtkBox::~vtkBox()
{
  delete this->BBox;
}

//----------------------------------------------------------------------------
// Set the bounds in various ways
void vtkBox::SetBounds(double xMin, double xMax,
                       double yMin, double yMax,
                       double zMin, double zMax)
{
  const double *minP = this->BBox->GetMinPoint();
  const double *maxP = this->BBox->GetMaxPoint();
  if ( (minP[0] == xMin) &&
       (maxP[0] == xMax) &&
       (minP[1] == yMin) &&
       (maxP[1] == yMax) &&
       (minP[2] == zMin) &&
       (maxP[2] == zMax))
    {
    return;
    }
  this->BBox->SetBounds(xMin, xMax, yMin, yMax, zMin, zMax);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBox::SetBounds(const double bounds[6])
{
  this->SetBounds(bounds[0],bounds[1], bounds[2],bounds[3],
                  bounds[4],bounds[5]);
}

//----------------------------------------------------------------------------
void vtkBox::SetXMin(double x, double y, double z)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting XMin to ("
                << x  << "," << y << "," << z << ")");
  const double *p = this->BBox->GetMinPoint();
  if ((p[0] == x) && (p[1] == y) && (p[2] == z))
    {
    return;
    }
  this->BBox->SetMinPoint(x, y, z);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBox::SetXMax(double x, double y, double z)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting XMax to ("
                << x  << "," << y << "," << z << ")");
  const double *p = this->BBox->GetMaxPoint();
  if ((p[0] == x) && (p[1] == y) && (p[2] == z))
    {
    return;
    }
  this->BBox->SetMaxPoint(x, y, z);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBox::GetBounds(double &xMin, double &xMax,
                       double &yMin, double &yMax,
                       double &zMin, double &zMax)
{
  this->BBox->GetBounds(xMin, xMax, yMin, yMax, zMin, zMax);
}

//----------------------------------------------------------------------------
void vtkBox::GetBounds(double bounds[6])
{
  this->BBox->GetBounds(bounds);
}

//----------------------------------------------------------------------------
double* vtkBox::GetBounds()
{
  this->BBox->GetBounds(this->Bounds);
  return this->Bounds;
}

//----------------------------------------------------------------------------
void vtkBox::AddBounds(const double bounds[6])
{
  vtkBoundingBox bbox(*(this->BBox));
  this->BBox->AddBounds(bounds);
  // If the unioned bounding has changed called modified
  if ((*this->BBox) != bbox)
    {
    this->Modified();
    }
}


//----------------------------------------------------------------------------
// Evaluate box equation. This differs from the similar vtkPlanes
// (with six planes) because of the "rounded" nature of the corners.
double vtkBox::EvaluateFunction(double x[3])
{
  double diff, dist, minDistance=(-VTK_DOUBLE_MAX), t, distance=0.0;
  int inside=1;
  const double *minP = this->BBox->GetMinPoint();
  const double *maxP = this->BBox->GetMaxPoint();

  for (int i=0; i<3; i++)
    {
    diff = this->BBox->GetLength(i);
    if ( diff != 0.0 )
      {
      t = (x[i]-minP[i]) / diff;
      if ( t < 0.0 )
        {
        inside = 0;
        dist = minP[i] - x[i];
        }
      else if ( t > 1.0 )
        {
        inside = 0;
        dist = x[i] - maxP[i];
        }
      else
        {//want negative distance, we are inside
        if ( t <= 0.5 )
          {
          dist = minP[i] - x[i];
          }
        else
          {
          dist = x[i] - maxP[i];
          }
        if ( dist > minDistance ) //remember, it's negative
          {
          minDistance = dist;
          }
        }//if inside
      }
    else
      {
      dist = fabs(x[i]-minP[i]);
      if (dist)
        {
        inside = 0;
        }
      }
    if ( dist > 0.0 )
      {
      distance += dist*dist;
      }
    }//for all coordinate directions

  distance = sqrt(distance);
  if ( inside )
    {
    return minDistance;
    }
  else
    {
    return distance;
    }
}

//----------------------------------------------------------------------------
// Evaluate box gradient.
void vtkBox::EvaluateGradient(double x[3], double n[3])
{
  int i, loc[3], minAxis=0;
  double dist, minDist=VTK_DOUBLE_MAX, center[3];
  double inDir[3], outDir[3];
  const double *minP = this->BBox->GetMinPoint();
  const double *maxP = this->BBox->GetMaxPoint();

  // Compute the location of the point with respect to the box.
  // Ultimately the point will lie in one of 27 separate regions around
  // or within the box. The gradient vector is computed differently in
  // each of the regions.
  inDir[0] = inDir[1] = inDir[2] = 0.0;
  outDir[0] = outDir[1] = outDir[2] = 0.0;
  this->BBox->GetCenter(center);
  for (i=0; i<3; i++)
    {
    if ( x[i] < minP[i] )
      {
      loc[i] = 0;
      outDir[i] = -1.0;
      }
    else if ( x[i] > maxP[i] )
      {
      loc[i] = 2;
      outDir[i] = 1.0;
      }
    else
      {
      loc[i] = 1;
      if ( x[i] <= center[i] )
        {
        dist = x[i] - minP[i];
        inDir[i] = -1.0;
        }
      else
        {
        dist = maxP[i] - x[i];
        inDir[i] = 1.0;
        }
      if ( dist < minDist ) //remember, it's negative
        {
        minDist = dist;
        minAxis = i;
        }
      }//if inside
    }//for all coordinate directions

  int indx = loc[0] + 3*loc[1] + 9*loc[2];

  switch (indx)
    {
    // verts - gradient points away from center point
    case 0: case 2: case 6: case 8: case 18: case 20: case 24: case 26:
      for (i=0; i<3; i++)
        {
        n[i] = x[i] - center[i];
        }
      vtkMath::Normalize(n);
      break;

    // edges - gradient points out from axis of cube
    case 1: case 3: case 5: case 7:
    case 9: case 11: case 15: case 17:
    case 19: case 21: case 23: case 25:
      for (i=0; i<3; i++)
        {
        if ( outDir[i] != 0.0 )
          {
          n[i] = x[i] - center[i];
          }
        else
          {
          n[i] = 0.0;
          }
        }
      vtkMath::Normalize(n);
      break;

    // faces - gradient points perpendicular to face
    case 4: case 10: case 12: case 14: case 16: case 22:
      for (i=0; i<3; i++)
        {
        n[i] = outDir[i];
        }
      break;

    // interior - gradient is perpendicular to closest face
    case 13:
      n[0] = n[1] = n[2] = 0.0;
      n[minAxis] = inDir[minAxis];
      break;
    default:
      assert("check: impossible case." && 0); // reaching this line is a bug.
      break;
    }
}

#define VTK_RIGHT 0
#define VTK_LEFT 1
#define VTK_MIDDLE 2

//----------------------------------------------------------------------------
// Bounding box intersection modified from Graphics Gems Vol I. The method
// returns a non-zero value if the bounding box is hit. Origin[3] starts
// the ray, dir[3] is the vector components of the ray in the x-y-z
// directions, coord[3] is the location of hit, and t is the parametric
// coordinate along line. (Notes: the intersection ray dir[3] is NOT
// normalized.  Valid intersections will only occur between 0<=t<=1.)
char vtkBox::IntersectBox (double bounds[6], double origin[3], double dir[3],
                           double coord[3], double& t)
{
  bool    inside=true;
  char    quadrant[3];
  int     i, whichPlane=0;
  double  maxT[3], candidatePlane[3];

  //  First find closest planes
  //
  for (i=0; i<3; i++)
    {
    if ( origin[i] < bounds[2*i] )
      {
      quadrant[i] = VTK_LEFT;
      candidatePlane[i] = bounds[2*i];
      inside = false;
      }
    else if ( origin[i] > bounds[2*i+1] )
      {
      quadrant[i] = VTK_RIGHT;
      candidatePlane[i] = bounds[2*i+1];
      inside = false;
      }
    else
      {
      quadrant[i] = VTK_MIDDLE;
      }
    }

  //  Check whether origin of ray is inside bbox
  //
  if (inside)
    {
    coord[0] = origin[0];
    coord[1] = origin[1];
    coord[2] = origin[2];
    t = 0;
    return 1;
    }

  //  Calculate parametric distances to plane
  //
  for (i=0; i<3; i++)
    {
    if ( quadrant[i] != VTK_MIDDLE && dir[i] != 0.0 )
      {
      maxT[i] = (candidatePlane[i]-origin[i]) / dir[i];
      }
    else
      {
      maxT[i] = -1.0;
      }
    }

  //  Find the largest parametric value of intersection
  //
  for (i=0; i<3; i++)
    {
    if ( maxT[whichPlane] < maxT[i] )
      {
      whichPlane = i;
      }
    }

  //  Check for valid intersection along line
  //
  if ( maxT[whichPlane] > 1.0 || maxT[whichPlane] < 0.0 )
    {
    return 0;
    }
  else
    {
    t = maxT[whichPlane];
    }

  //  Intersection point along line is okay.  Check bbox.
  //
  for (i=0; i<3; i++)
    {
    if (whichPlane != i)
      {
      coord[i] = origin[i] + maxT[whichPlane]*dir[i];
      if ( coord[i] < bounds[2*i] || coord[i] > bounds[2*i+1] )
        {
        return 0;
        }
      }
    else
      {
      coord[i] = candidatePlane[i];
      }
    }

  return 1;
}
#undef VTK_RIGHT
#undef VTK_LEFT
#undef VTK_MIDDLE

//----------------------------------------------------------------------------
// Bounding box intersection code from David Gobbi.  Go through the
// bounding planes one at a time and compute the parametric coordinate
// of each intersection.
int vtkBox::IntersectWithLine(const double bounds[6],
                              const double p1[3], const double p2[3],
                              double &t1, double &t2,
                              double x1[3], double x2[3],
                              int &plane1, int &plane2)
{
  plane1 = -1;
  plane2 = -1;
  t1 = 0.0;
  t2 = 1.0;

  for (int j = 0; j < 3; j++)
    {
    for (int k = 0; k < 2; k++)
      {
      // Compute distances of p1 and p2 from the plane along the plane normal
      int i = 2*j + k;
      double d1 = (bounds[i] - p1[j])*(1 - 2*k);
      double d2 = (bounds[i] - p2[j])*(1 - 2*k);

      // If both distances are positive, both points are outside
      if (d1 > 0 && d2 > 0)
        {
        return 0;
        }
      // If one of the distances is positive, the line crosses the plane
      else if (d1 > 0 || d2 > 0)
        {
        // Compute fractional distance "t" of the crossing between p1 & p2
        double t = 0.0;
        if (d1 != 0)
          {
          t = d1/(d1 - d2);
          }

        // If point p1 was clipped, adjust t1
        if (d1 > 0)
          {
          if (t >= t1)
            {
            t1 = t;
            plane1 = i;
            }
          }
        // else point p2 was clipped, so adjust t2
        else
          {
          if (t <= t2)
            {
            t2 = t;
            plane2 = i;
            }
          }

        // If this happens, there's no line left
        if (t1 > t2)
          {
          // Allow for planes that are coincident or slightly inverted
          if (plane1 < 0 || plane2 < 0 || (plane1 >> 1) != (plane2 >> 1))
            {
            return 0;
            }
          }
        }
      }
    }

  double *x = x1;
  double t = t1;
  int plane = plane1;

  for (int count = 0; count < 2; count++)
    {
    if (x)
      {
      for (int i = 0; i < 3; i++)
        {
        if (plane == 2*i || plane == 2*i+1)
          {
          x[i] = bounds[plane];
          }
        else
          {
          x[i] = p1[i]*(1.0 - t) + p2[i]*t;
          if (x[i] < bounds[2*i]) { x[i] = bounds[2*i]; }
          if (x[i] > bounds[2*i+1]) { x[i] = bounds[2*i+1]; }
          }
        }
      }

      x = x2;
      t = t2;
      plane = plane2;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkBox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  const double *minP = this->BBox->GetMinPoint();
  const double *maxP = this->BBox->GetMaxPoint();

  os << indent << "XMin: (" << minP[0] << ", "
               << minP[1] << ", " << minP[2] << ")\n";
  os << indent << "XMax: (" << maxP[0] << ", "
               << maxP[1] << ", " << maxP[2] << ")\n";
}
//----------------------------------------------------------------------------
void vtkBox::GetXMin(double p[3])
{
  this->BBox->GetMinPoint(p[0], p[1], p[2]);
}

//----------------------------------------------------------------------------
void vtkBox::GetXMin(double &x , double &y, double &z)
{
  this->BBox->GetMinPoint(x, y, z);
}

//----------------------------------------------------------------------------
void vtkBox::GetXMax(double p[3])
{
  this->BBox->GetMaxPoint(p[0], p[1], p[2]);
}

//----------------------------------------------------------------------------
void vtkBox::GetXMax(double &x , double &y, double &z)
{
  this->BBox->GetMaxPoint(x, y, z);
}
