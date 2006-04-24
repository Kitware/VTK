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

vtkCxxRevisionMacro(vtkBox, "1.6");
vtkStandardNewMacro(vtkBox);

// Construct the box centered at the origin and each side length 1.0.
//----------------------------------------------------------------------------
vtkBox::vtkBox()
{
  this->XMin[0] = -0.5;
  this->XMin[1] = -0.5;
  this->XMin[2] = -0.5;

  this->XMax[0] =  0.5;
  this->XMax[1] =  0.5;
  this->XMax[2] =  0.5;
}

//----------------------------------------------------------------------------
// Set the bounds in various ways
void vtkBox::SetBounds(double xMin, double xMax,
                       double yMin, double yMax,
                       double zMin, double zMax)
{
  if ( this->XMin[0] != xMin || this->XMax[0] != xMax || 
       this->XMin[1] != yMin || this->XMax[1] != yMax || 
       this->XMin[2] != zMin || this->XMax[2] != yMax )
    {
    this->XMin[0] = xMin;
    this->XMax[0] = xMax;
    this->XMin[1] = yMin;
    this->XMax[1] = yMax;
    this->XMin[2] = zMin;
    this->XMax[2] = zMax;
    for (int i=0; i<3; i++)
      {
      if ( this->XMax[i] < this->XMin[i] )
        {
        this->XMax[i] = this->XMin[i];
        }
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkBox::SetBounds(double bounds[6])
{
  this->SetBounds(bounds[0],bounds[1], bounds[2],bounds[3], 
                  bounds[4],bounds[5]);
}

//----------------------------------------------------------------------------
void vtkBox::GetBounds(double &xMin, double &xMax,
                       double &yMin, double &yMax,
                       double &zMin, double &zMax)
{
  xMin = this->XMin[0];
  yMin = this->XMin[1];
  zMin = this->XMin[2];
  xMax = this->XMax[0];
  yMax = this->XMax[1];
  zMax = this->XMax[2];
}

//----------------------------------------------------------------------------
void vtkBox::GetBounds(double bounds[6])
{
  for (int i=0; i<3; i++)
    {
    bounds[2*i] = this->XMin[i];
    bounds[2*i+1] = this->XMax[i];
    }
}

//----------------------------------------------------------------------------
double* vtkBox::GetBounds()
{
  this->GetBounds(this->Bounds);
  return this->Bounds;
}

//----------------------------------------------------------------------------
void vtkBox::AddBounds(double bounds[6])
{
  int i, idx;
  for (i=0; i<3; i++)
    {
    idx = 2*i;
    if ( bounds[idx] < this->XMin[i] )
      {
      this->XMin[i] = bounds[idx];
      }
    if ( bounds[idx+1] > this->XMax[i] )
      {
      this->XMax[i] = bounds[idx+1];
      }
    }
}


//----------------------------------------------------------------------------
// Evaluate box equation. This differs from the similar vtkPlanes
// (with six planes) because of the "rounded" nature of the corners.
double vtkBox::EvaluateFunction(double x[3])
{
  double diff, dist, minDistance=(-VTK_DOUBLE_MAX), t, distance=0.0;
  int inside=1;
  for (int i=0; i<3; i++)
    {
    diff = this->XMax[i] - this->XMin[i];
    if ( diff != 0.0 )
      {
      t = (x[i]-this->XMin[i]) / (this->XMax[i]-this->XMin[i]);
      if ( t < 0.0 )
        {
        inside = 0;
        dist = this->XMin[i] - x[i];
        }
      else if ( t > 1.0 )
        {
        inside = 0;
        dist = x[i] - this->XMax[i];
        }
      else
        {//want negative distance, we are inside
        if ( t <= 0.5 )
          {
          dist = this->XMin[i] - x[i];
          }
        else
          {
          dist = x[i] - this->XMax[i];
          }
        if ( dist > minDistance ) //remember, it's negative
          {
          minDistance = dist;
          }
        }//if inside
      }
    else
      {
      dist = fabs(x[i]-this->XMin[i]);
      if ( x[i] != this->XMin[i] )
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
  
  // Compute the location of the point with respect to the box.
  // Ultimately the point will lie in one of 27 separate regions around
  // or within the box. The gradient vector is computed differently in
  // each of the regions.
  inDir[0] = inDir[1] = inDir[2] = 0.0;
  outDir[0] = outDir[1] = outDir[2] = 0.0;
  for (i=0; i<3; i++)
    {
    center[i] = (this->XMin[i] + this->XMax[i])/2.0;
    if ( x[i] < this->XMin[i] )
      {
      loc[i] = 0;
      outDir[i] = -1.0;
      }
    else if ( x[i] > this->XMax[i] )
      {
      loc[i] = 2;
      outDir[i] = 1.0;
      }
    else
      {
      loc[i] = 1;
      if ( x[i] <= center[i] )
        {
        dist = x[i] - this->XMin[i];
        inDir[i] = -1.0;
        }
      else
        {
        dist = this->XMax[i] - x[i];
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
  char    inside=1;
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
      inside = 0;
      }
    else if ( origin[i] > bounds[2*i+1] ) 
      {
      quadrant[i] = VTK_RIGHT;
      candidatePlane[i] = bounds[2*i+1];
      inside = 0;
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
void vtkBox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "XMin: (" << this->XMin[0] << ", " 
               << this->XMin[1] << ", " << this->XMin[2] << ")\n";
  os << indent << "XMax: (" << this->XMax[0] << ", " 
               << this->XMax[1] << ", " << this->XMax[2] << ")\n";
}
