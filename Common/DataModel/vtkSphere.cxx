/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphere.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSphere.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSphere);

//----------------------------------------------------------------------------
// Construct sphere with center at (0,0,0) and radius=0.5.
vtkSphere::vtkSphere()
{
  this->Radius = 0.5;

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
}

//----------------------------------------------------------------------------
// Evaluate sphere equation ((x-x0)^2 + (y-y0)^2 + (z-z0)^2) - R^2.
double vtkSphere::EvaluateFunction(double x[3])
{
  return ( ((x[0] - this->Center[0]) * (x[0] - this->Center[0]) +
           (x[1] - this->Center[1]) * (x[1] - this->Center[1]) +
           (x[2] - this->Center[2]) * (x[2] - this->Center[2])) -
           this->Radius*this->Radius );
}

//----------------------------------------------------------------------------
// Evaluate sphere gradient.
void vtkSphere::EvaluateGradient(double x[3], double n[3])
{
  n[0] = 2.0 * (x[0] - this->Center[0]);
  n[1] = 2.0 * (x[1] - this->Center[1]);
  n[2] = 2.0 * (x[2] - this->Center[2]);
}

// The following methods are used to compute bounding spheres.
//
#define VTK_ASSIGN_POINT(_x,_y) {_x[0]=_y[0];_x[1]=_y[1];_x[2]=_y[2];}
//----------------------------------------------------------------------------
// Inspired by Graphics Gems Vol. I ("An Efficient Bounding Sphere" by Jack Ritter).
// The algorithm works in two parts: first an initial estimate of the largest sphere;
// second an adjustment to the sphere to make sure that it includes all the points.
// Typically this returns a bounding sphere that is ~5% larger than the minimal
// bounding sphere.
template <class T>
void vtkSphereComputeBoundingSphere(T *pts, vtkIdType numPts, T sphere[4],
                                    vtkIdType hints[2])
{
  sphere[0] = sphere[1] = sphere[2] = sphere[3] = 0.0;
  if ( numPts < 1 )
  {
    return;
  }

  vtkIdType i;
  T *p, d1[3], d2[3];
  if ( hints )
  {
    p = pts + 3*hints[0];
    VTK_ASSIGN_POINT(d1,p);
    p = pts + 3*hints[1];
    VTK_ASSIGN_POINT(d2,p);
  }
  else //no hints provided, compute an initial guess
  {
    T xMin[3], xMax[3], yMin[3], yMax[3], zMin[3], zMax[3];
    xMin[0] = xMin[1] = xMin[2] = VTK_FLOAT_MAX;
    yMin[0] = yMin[1] = yMin[2] = VTK_FLOAT_MAX;
    zMin[0] = zMin[1] = zMin[2] = VTK_FLOAT_MAX;
    xMax[0] = xMax[1] = xMax[2] = -VTK_FLOAT_MAX;
    yMax[0] = yMax[1] = yMax[2] = -VTK_FLOAT_MAX;
    zMax[0] = zMax[1] = zMax[2] = -VTK_FLOAT_MAX;

    // First part: Estimate the points furthest apart to define the largest sphere.
    // Find the points that span the greatest distance on the x-y-z axes. Use these
    // two points to define a sphere centered between the two points.
    for (p=pts, i=0; i<numPts; ++i, p+=3)
    {
      if (p[0] < xMin[0] ) VTK_ASSIGN_POINT(xMin,p);
      if (p[0] > xMax[0] ) VTK_ASSIGN_POINT(xMax,p);
      if (p[1] < yMin[1] ) VTK_ASSIGN_POINT(yMin,p);
      if (p[1] > yMax[1] ) VTK_ASSIGN_POINT(yMax,p);
      if (p[2] < zMin[2] ) VTK_ASSIGN_POINT(zMin,p);
      if (p[2] > zMax[2] ) VTK_ASSIGN_POINT(zMax,p);
    }
    T xSpan = (xMax[0]-xMin[0])*(xMax[0]-xMin[0]) + (xMax[1]-xMin[1])*(xMax[1]-xMin[1]) +
      (xMax[2]-xMin[2])*(xMax[2]-xMin[2]);
    T ySpan = (yMax[0]-yMin[0])*(yMax[0]-yMin[0]) + (yMax[1]-yMin[1])*(yMax[1]-yMin[1]) +
      (yMax[2]-yMin[2])*(yMax[2]-yMin[2]);
    T zSpan = (zMax[0]-zMin[0])*(zMax[0]-zMin[0]) + (zMax[1]-zMin[1])*(zMax[1]-zMin[1]) +
      (zMax[2]-zMin[2])*(zMax[2]-zMin[2]);

    if ( xSpan > ySpan )
    {
      if ( xSpan > zSpan )
      {
        VTK_ASSIGN_POINT(d1,xMin);
        VTK_ASSIGN_POINT(d2,xMax);
      }
      else
      {
        VTK_ASSIGN_POINT(d1,zMin);
        VTK_ASSIGN_POINT(d2,zMax);
      }
    }
    else //ySpan > xSpan
    {
      if ( ySpan > zSpan )
      {
        VTK_ASSIGN_POINT(d1,yMin);
        VTK_ASSIGN_POINT(d2,yMax);
      }
      else
      {
        VTK_ASSIGN_POINT(d1,zMin);
        VTK_ASSIGN_POINT(d2,zMax);
      }
    }
  }//no hints provided

  // Compute initial estimated sphere
  sphere[0] = (d1[0]+d2[0]) / 2.0;
  sphere[1] = (d1[1]+d2[1]) / 2.0;
  sphere[2] = (d1[2]+d2[2]) / 2.0;
  T r2 = vtkMath::Distance2BetweenPoints(d1,d2)/4.0;
  sphere[3] = sqrt(r2);

  // Second part: Make a pass over the points to make sure that they fit inside the sphere.
  // If not, adjust the sphere to fit the point.
  T dist, dist2, delta;
  for (p=pts, i=0; i<numPts; ++i, p+=3)
  {
    dist2 = vtkMath::Distance2BetweenPoints(p,sphere);
    if ( dist2 > r2 )
    {
      dist = sqrt(dist2);
      sphere[3] = (sphere[3] + dist) / 2.0;
      r2 = sphere[3]*sphere[3];
      delta = dist - sphere[3];
      sphere[0] = (sphere[3]*sphere[0] + delta*p[0]) / dist;
      sphere[1] = (sphere[3]*sphere[1] + delta*p[1]) / dist;
      sphere[2] = (sphere[3]*sphere[2] + delta*p[2]) / dist;
    }
  }

}
#undef VTK_ASSIGN_POINT

#define VTK_ASSIGN_SPHERE(_x,_y) {_x[0]=_y[0];_x[1]=_y[1];_x[2]=_y[2];_x[3]=_y[3];}
// An approximation to the bounding sphere of a set of spheres. The algorithm
// creates an iniitial approximation from two spheres that are expected to be
// the farthest apart (taking into accout their radius). A second pass may
// grow the bounding sphere if the remaining spheres are not contained within
// it. The hints[2] array indicates two spheres that are expected to be the
// farthest apart.
//----------------------------------------------------------------------------
template <class T>
void vtkSphereComputeBoundingSphere(T **spheres, vtkIdType numSpheres, T sphere[4],
                                    vtkIdType hints[2])
{
  if ( numSpheres < 1 )
  {
    sphere[0] = sphere[1] = sphere[2] = sphere[3] = 0.0;
    return;
  }
  else if ( numSpheres == 1 )
  {
    VTK_ASSIGN_SPHERE(sphere,spheres[0]);
    return;
  }

  // Okay two or more spheres
  vtkIdType i, j;
  T *s, s1[4], s2[4];
  if ( hints )
  {
    s = spheres[hints[0]];
    VTK_ASSIGN_SPHERE(s1,s);
    s = spheres[hints[1]];
    VTK_ASSIGN_SPHERE(s2,s);
  }
  else //no hints provided, compute an initial guess
  {
    T xMin[4], xMax[4], yMin[4], yMax[4], zMin[4], zMax[4];
    xMin[0] = xMin[1] = xMin[2] = xMin[3] = VTK_FLOAT_MAX;
    yMin[0] = yMin[1] = yMin[2] = yMin[3] = VTK_FLOAT_MAX;
    zMin[0] = zMin[1] = zMin[2] = zMin[3] = VTK_FLOAT_MAX;
    xMax[0] = xMax[1] = xMax[2] = xMax[3] = -VTK_FLOAT_MAX;
    yMax[0] = yMax[1] = yMax[2] = yMax[3] = -VTK_FLOAT_MAX;
    zMax[0] = zMax[1] = zMax[2] = zMax[3] = -VTK_FLOAT_MAX;

    // First part: Estimate the points furthest apart to define the largest sphere.
    // Find the points that span the greatest distance on the x-y-z axes. Use these
    // two points to define a sphere centered between the two points.
    for (i=0; i<numSpheres; ++i)
    {
      s = spheres[i];
      if ((s[0]-s[3]) < xMin[0] ) VTK_ASSIGN_SPHERE(xMin,s);
      if ((s[0]+s[3]) > xMax[0] ) VTK_ASSIGN_SPHERE(xMax,s);
      if ((s[1]-s[3]) < yMin[1] ) VTK_ASSIGN_SPHERE(yMin,s);
      if ((s[1]+s[3]) > yMax[1] ) VTK_ASSIGN_SPHERE(yMax,s);
      if ((s[2]-s[3]) < zMin[2] ) VTK_ASSIGN_SPHERE(zMin,s);
      if ((s[2]+s[3]) > zMax[2] ) VTK_ASSIGN_SPHERE(zMax,s);
    }
    T xSpan = (xMax[0]+xMax[3]-xMin[0]-xMin[3])*(xMax[0]+xMax[3]-xMin[0]-xMin[3]) +
      (xMax[1]+xMax[3]-xMin[1]-xMin[3])*(xMax[1]+xMax[3]-xMin[1]-xMin[3]) +
      (xMax[2]+xMax[3]-xMin[2]-xMin[3])*(xMax[2]+xMax[3]-xMin[2]-xMin[3]);
    T ySpan = (yMax[0]+yMax[3]-yMin[0]-yMin[3])*(yMax[0]+yMax[3]-yMin[0]-yMin[3]) +
      (yMax[1]+yMax[3]-yMin[1]-yMin[3])*(yMax[1]+yMax[3]-yMin[1]-yMin[3]) +
      (yMax[2]+yMax[3]-yMin[2]-yMin[3])*(yMax[2]+yMax[3]-yMin[2]-yMin[3]);
    T zSpan = (zMax[0]+zMax[3]-zMin[0]-zMin[3])*(zMax[0]+zMax[3]-zMin[0]-zMin[3]) +
      (zMax[1]+zMax[3]-zMin[1]-zMin[3])*(zMax[1]+zMax[3]-zMin[1]-zMin[3]) +
      (zMax[2]+zMax[3]-zMin[2]-zMin[3])*(zMax[2]+zMax[3]-zMin[2]-zMin[3]);

    if ( xSpan > ySpan )
    {
      if ( xSpan > zSpan )
      {
        VTK_ASSIGN_SPHERE(s1,xMin);
        VTK_ASSIGN_SPHERE(s2,xMax);
      }
      else
      {
        VTK_ASSIGN_SPHERE(s1,zMin);
        VTK_ASSIGN_SPHERE(s2,zMax);
      }
    }
    else //ySpan > xSpan
    {
      if ( ySpan > zSpan )
      {
        VTK_ASSIGN_SPHERE(s1,yMin);
        VTK_ASSIGN_SPHERE(s2,yMax);
      }
      else
      {
        VTK_ASSIGN_SPHERE(s1,zMin);
        VTK_ASSIGN_SPHERE(s2,zMax);
      }
    }
  }//no hints provided

  // Compute initial estimated sphere, take into account the radius of each sphere
  T tmp, v[3], r2 = vtkMath::Distance2BetweenPoints(s1,s2)/4.0;
  sphere[3] = sqrt(r2);
  T t1 = -s1[3]/(2.0*sphere[3]);
  T t2 = 1.0 + s2[3]/(2.0*sphere[3]);
  for (i=0; i<3; ++i)
  {
    v[i] = s2[i] - s1[i];
    tmp = s1[i] + t1*v[i];
    s2[i] = s1[i] + t2*v[i];
    s1[i] = tmp;
    sphere[i] = (s1[i]+s2[i]) / 2.0;
  }
  r2 = vtkMath::Distance2BetweenPoints(s1,s2)/4.0;
  sphere[3] = sqrt(r2);

  // Second part: Make a pass over the points to make sure that they fit inside the sphere.
  // If not, adjust the sphere to fit the point.
  T dist, dist2, fac, sR2;
  for (i=0; i<numSpheres; ++i)
  {
    s = spheres[i];
    sR2 = s[3]*s[3];
    dist2 = vtkMath::Distance2BetweenPoints(s,sphere);
    if ( sR2 > dist2 ) //approximation to avoid square roots if possible
    {
      fac = 2.0*sR2;
    }
    else
    {
      fac = 2.0*dist2;
    }
    if ( (dist2 + fac + sR2) > r2 ) //approximate test
    {
      dist = sqrt(dist2);
      if ( ((dist+s[3])*(dist+s[3])) > r2 ) //more accurate test
      {
        for (j=0; j<3; ++j)
        {
          v[j] = s[j] - sphere[j];
          s1[j] = sphere[j] - (sphere[3]/dist)*v[j];
          s2[j] = sphere[j] + (1.0+s[3]/dist)*v[j];
          sphere[j] = (s1[j]+s2[j]) / 2.0;
        }
        r2 = vtkMath::Distance2BetweenPoints(s1,s2)/4.0;
        sphere[3] = sqrt(r2);
      }
    }
  }

}
#undef VTK_ASSIGN_SPHERE

// Type specific wrappers for the templated functions below
//----------------------------------------------------------------------------
void vtkSphere::ComputeBoundingSphere(float *pts, vtkIdType numPts, float sphere[4],
                                      vtkIdType hints[2])
{
  vtkSphereComputeBoundingSphere(pts,numPts,sphere,hints);
}
//----------------------------------------------------------------------------
void vtkSphere::ComputeBoundingSphere(double *pts, vtkIdType numPts, double sphere[4],
                                      vtkIdType hints[2])
{
  vtkSphereComputeBoundingSphere(pts,numPts,sphere,hints);
}
//----------------------------------------------------------------------------
void vtkSphere::ComputeBoundingSphere(float **spheres, vtkIdType numSpheres, float sphere[4],
                                      vtkIdType hints[2])
{
  vtkSphereComputeBoundingSphere(spheres,numSpheres,sphere,hints);
}
//----------------------------------------------------------------------------
void vtkSphere::ComputeBoundingSphere(double **spheres, vtkIdType numSpheres, double sphere[4],
                                      vtkIdType hints[2])
{
  vtkSphereComputeBoundingSphere(spheres,numSpheres,sphere,hints);
}

//----------------------------------------------------------------------------
void vtkSphere::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", "
     << this->Center[1] << ", " << this->Center[2] << ")\n";
}
