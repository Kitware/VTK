/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereTree.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <stdio.h>
#include "vtkSphereTree.h"
#include "vtkPolyData.h"
#include "vtkMath.h"
//----------------------------------------------------------------------------
vtkSphereTree::vtkSphereTree()
{
  this->MaximumRadius = 10000;
  this->Tolerance = 10000;
  this->Inside = 1;
  this->Spheres = NULL;
  this->Points = NULL;
}

//----------------------------------------------------------------------------
void vtkSphereTree::Execute()
{
  vtkCellArray *inPolys;
  int numPolys;
  int numSpheres;
  int npts, *pts;
  vtkFloatPoints *newPoints;
  vtkFloatScalars *newScalars;
  float *p0, *p1, *p2;
  vtkPolyData *input =(vtkPolyData *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;
  vtkPointData *pointData = output->GetPointData(); 
  vtkSphereTreeSphere *sphere;
  float center[3];
  float radius;
  int idx;
  
  
  // Initialize
  this->Points = input->GetPoints();
  this->NewPoints = new vtkFloatPoints();

  // Get the polygons that will be converted to spheres
  inPolys = input->GetPolys();
  numPolys = inPolys->GetNumberOfCells();
  //inStrips = input->GetStrips();
  
  // Polygons
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    printf("%d\n", numPolys);
    --numPolys;
    if (npts != 3)
      {
      vtkWarningMacro(<< "Can only handle triangles.");
      }
    else
      {
      p0 = this->Points->GetPoint(pts[0]);
      p1 = this->Points->GetPoint(pts[1]);
      p2 = this->Points->GetPoint(pts[2]);
      this->TriangleExecute(p0, p1, p2);
      }
    }

  // Convert spheres to PolyData for output.
  // Allocate
  numSpheres = this->GetNumberOfSpheres();
  newPoints = new vtkFloatPoints(numSpheres);
  newScalars = new vtkFloatScalars(numSpheres);
  printf("%d\n", numSpheres);
  
  sphere = this->Spheres;
  while (sphere)
    {
    for (idx = 0; idx < 3; ++idx)
      {
      center[idx] = sphere->TriangleCentroid[idx] + 
	(sphere->K * sphere->TriangleNormal[idx]);
      }
    radius = sqrt(sphere->TriangleRadiusSquared + sphere->K * sphere->K);
    newPoints->InsertNextPoint(center);
    newScalars->InsertNextScalar(radius);
    sphere = sphere->Next;
    }
  
  // Polygons
  // Update self and release memory
  output->SetPoints(newPoints);
  newPoints->Delete();

  pointData->SetScalars(newScalars);
  newScalars->Delete();

  this->CleanUp();
  
  vtkDebugMacro(<< "Finished executing.");
}


  
//----------------------------------------------------------------------------
void vtkSphereTree::TriangleExecute(float *p0, float *p1, float *p2)
{
  float centroid[3], normal[3];
  float radiusSquared;

  if (this->ComputeTriangleInfo(p0, p1, p2, centroid, normal, radiusSquared))
    {
    vtkDebugMacro("AcuteTriangle: (" << p0[0] << ", " << p0[1] << ", " << p0[2]
		  << "), (" << p1[0] << ", " << p1[1] << ", " << p1[2]
		  << "), (" << p2[0] << ", " << p2[1] << ", " << p2[2] 
		  << "), centroid (" << centroid[0] << ", " << centroid[1] << ", " 
		  << centroid[2] << ")");
    if ( ! this->AddSphere(centroid, normal, radiusSquared, p0, p1, p2))
      {
      this->BigTriangleExecute(p0, p1, p2);
      }
    }
}



//----------------------------------------------------------------------------
// Makes multiple spheres for large triangles.
void vtkSphereTree::BigTriangleExecute(float *p0, float *p1, float *p2)
{
  float temp, d01, d12, d02;
  int idx;
  
  // Determine the longest edge.
  d01 = d12 = d02 = 0.0;
  for (idx = 0; idx < 3; ++idx)
    {
    temp = p1[idx] - p0[idx];
    d01 += temp * temp;
    temp = p2[idx] - p1[idx];
    d12 += temp * temp;
    temp = p2[idx] - p0[idx];
    d02 += temp * temp;
    }
  
  if ((d01 >= d02) && (d01 >= d12))
    {
    this->SplitTriangle(p2, p0, p1);
    }
  else if ((d12 > d02) && (d12 > d01))
    {
    this->SplitTriangle(p0, p1, p2);
    }
  else if ((d02 > d01) && (d02 >= d12))
    {
    this->SplitTriangle(p1, p2, p0);
    }
}

//----------------------------------------------------------------------------
// Splits a triangle into two.  It assumes that p1 - p2 is the longest edge.
void vtkSphereTree::SplitTriangle(float *p0, float *p1, float *p2)
{  
  int idx;
  float newPoint[3];

  vtkDebugMacro(<< "Splitting triangle");
  for (idx = 0; idx < 3; ++idx)
    {
    newPoint[idx] = (p1[idx] + p2[idx]) * 0.5;
    }
  
  this->AddPoint(newPoint);
  this->TriangleExecute(p0, p1, newPoint);
  this->TriangleExecute(p0, newPoint, p2);
}

  


  
  
  
//----------------------------------------------------------------------------
// Computes the centroid and normal of a triangle.
// It returns 0 if the three points are colinear.
int vtkSphereTree::ComputeTriangleInfo(float *p0, float *p1, float *p2,
				       float *centroid, float *normal,
				       float &radiusSquared)
{
  float a[3], b[3], segNormal[3], segMiddle[3], segRadiusSquared;
  float k;
  int idx;
  float d01, d12, d02;
  float a0, a1, a2;

  // Compute the normal of the triangle ...
  for (idx = 0; idx < 3; ++idx)
    {
    a[idx] = p1[idx] - p0[idx];
    b[idx] = p2[idx] - p0[idx];
    }
  vtkMath::Cross(a, b, normal);
  if (vtkMath::Normalize(normal) == 0.0)
    {
    vtkWarningMacro(<< "Could not compute normal"
       << "\n     P0: " << p0[0] << ", " << p0[1] << ", " << p0[2]
       << "\n     P1: " << p1[0] << ", " << p1[1] << ", " << p1[2]
       << "\n     P2: " << p2[0] << ", " << p2[1] << ", " << p2[2]);
    return 0;
    }
  
  // First check for an obtuse triangle
  a0 = a1 = a2 = 0.0;
  for (idx = 0; idx < 3; ++idx)
    {
    d01 = p1[idx] - p0[idx];
    d02 = p2[idx] - p0[idx];
    d12 = p2[idx] - p1[idx];
    a0 += d01 * d02;
    a1 -= d01 * d12;
    a2 += d02 * d12;
    }
  if (a0 < 0.0)
    {
    this->ComputeSegmentInfo(p1, p2, centroid, radiusSquared);
    return 1;
    }
  if (a1 < 0.0)
    {
    this->ComputeSegmentInfo(p2, p0, centroid, radiusSquared);
    return 1;
    }
  if (a2 < 0.0)
    {
    this->ComputeSegmentInfo(p0, p1, centroid, radiusSquared);
    return 1;
    }
  
  // Traingle is acute. Use centroid  
  this->ComputeSegmentInfo(p0, p1, segMiddle, segRadiusSquared);
  
  // Find normal of p0->p1 segment (a).
  vtkMath::Cross(a, normal, segNormal);
  if (vtkMath::Normalize(segNormal) == 0.0)
    {
    vtkWarningMacro(<< "Could not compute segment normal"
       << "\n     P0: " << p0[0] << ", " << p0[1] << ", " << p0[2]
       << "\n     P1: " << p1[0] << ", " << p1[1] << ", " << p1[2]
       << "\n     P2: " << p2[0] << ", " << p2[1] << ", " << p2[2]);
    return 0;
    }

  // Find the triangle centroid from the segment centroid (middle)
  k = ComputeNewCentroid(segMiddle, segNormal, segRadiusSquared, p2);
  for (idx = 0; idx < 3; ++idx)
    {
    centroid[idx] = segMiddle[idx] + k * segNormal[idx];
    }
  
  // Compute the triangle radius squared
  radiusSquared = k*k + segRadiusSquared;
  
  return 1;
}


//----------------------------------------------------------------------------
// Computes the middle and radius squared of a segment.
void vtkSphereTree::ComputeSegmentInfo(float *p0, float *p1, float *center,
				       float &radiusSquared)
{
  float temp;
  int idx;
  
  radiusSquared = 0.0;
  for (idx = 0; idx < 3; ++idx)
    {
    center[idx] = (p0[idx] + p1[idx]) * 0.5;
    temp = p0[idx] - p1[idx];
    radiusSquared += temp * temp;
    }
  
  // convert diameter squared into radius squared
  radiusSquared *= 0.25;
}

  




  
//----------------------------------------------------------------------------
// Given triangle/Segment information (centroid, normal, radiusSquared), 
// and a new point, compute and return the factor K which can be used
// to compute the information of the new primative (tetrahedron/triangle).
// newCentroid = centroid + K * normal;
// newRadiusSquared = radiusSquared + K*K;
// newNormal must be computed some other way.
// No arguments are modified.
float vtkSphereTree::ComputeNewCentroid(float *centroid, float *normal,
					float radiusSquared, float *point)
{
  float a, b, temp;
  int idx;
  
  a = b = 0.0;
  for (idx = 0; idx < 3; ++idx)
    {
    temp = centroid[idx] - point[idx];
    a += temp * temp;
    b += normal[idx] * temp;
    }
  if (b == 0.0)
    {
    // vtkErrorMacro(<< "Could not compute new centroid");
    return this->MaximumRadius;
    }
  
  return (radiusSquared - a) / (2.0 * b);
}




//----------------------------------------------------------------------------
// Adds a point which will shrink any previously created spheres and
// limit the radius of any future sphere.
// It is always tricky removing elements from the middle of a linked list
void vtkSphereTree::AddPoint(float *point)
{
  vtkSphereTreeSphere **previous;
  vtkSphereTreeSphere *tempSphere;
  vtkSphereTreeSphere *sphere;
  vtkSphereTreeSphere *remove = NULL;
  float dSquared, temp, kTolerance;
  

  this->NewPoints->InsertNextPoint(point);

  // look at every sphere to find if sphere is still ok.
  previous = &(this->Spheres);
  sphere = this->Spheres;
  while (sphere)
    {
    // Compute distance squared
    temp = sphere->Center[0] - point[0];
    dSquared = temp * temp;
    temp = sphere->Center[1] - point[1];
    dSquared += temp * temp;
    temp = sphere->Center[2] - point[2];
    dSquared += temp * temp;
    if (dSquared < sphere->K * sphere->K)
      {
      tempSphere = sphere;
      // Remove the sphere from the global list..
      *previous = sphere->Next;
      // increment this loop to the next sphere
      sphere = sphere->Next;
      // Add this sphere to the remove list.
      tempSphere->Next = remove;
      remove = tempSphere;
      }
    else
      {
      // increment this loop to the next sphere
      previous = &(sphere->Next);
      sphere = sphere->Next;
      }
    }

  // Remake every sphere to be removed.
  sphere = remove;
  while (sphere)
    {
    //this->TriangleExecute(sphere->P0, sphere->P1, sphere->P2);
    
    // Only one point in sphere.
    // Computed the minimum value of k allowed by tolerance.
    kTolerance = (sphere->TriangleRadiusSquared - this->Tolerance*this->Tolerance)
      / (2.0 * this->Tolerance);
  
    temp = this->ComputeNewCentroid(sphere->TriangleCentroid, 
				    sphere->TriangleNormal,
				    sphere->TriangleRadiusSquared, point);
    if (temp < 0.0)
      {
      temp = 0.0;
      }
    sphere->K = temp;
    // Check for tolerance violation
    if (sphere->K < kTolerance)
      {
      // Sphere needs to be split
      this->BigTriangleExecute(sphere->P0, sphere->P1, sphere->P2);
      }
    else
      {
      // Sphere with ne radius ... is OK.
      // Compute new center and radiusSquared
      sphere->Center[0] = sphere->TriangleCentroid[0] 
	+ sphere->K * sphere->TriangleNormal[0];
      sphere->Center[1] = sphere->TriangleCentroid[1] 
	+ sphere->K * sphere->TriangleNormal[1];
      sphere->Center[2] = sphere->TriangleCentroid[2] 
	+ sphere->K * sphere->TriangleNormal[2];
      this->MakeSphere(sphere->P0, sphere->P1, sphere->P2, 
		       sphere->TriangleCentroid, sphere->TriangleNormal,
		       sphere->TriangleRadiusSquared, sphere->K,
		       sphere->Center);
      }
    
    tempSphere = sphere;
    sphere = sphere->Next;
    delete tempSphere;
    }
}

  



//----------------------------------------------------------------------------
// Creates a sphere if tolerance is not violated.  
// Returns 0 otherwise.
vtkSphereTreeSphere *vtkSphereTree::AddSphere(float *triangleCentroid, 
					      float *triangleNormal,
					      float triangleRadiusSquared,
					      float *p0, float *p1, float *p2)
{
  int idxPoint, numPoints;
  float point[3], kMin, radiusSquared;
  float dSquared, center[3], temp;
  float kTolerance;
  float d[3];

  // Flip normal if we are placing spheres on the inside
  if (this->Inside)
    {
    triangleNormal[0] = -triangleNormal[0];
    triangleNormal[1] = -triangleNormal[1];
    triangleNormal[2] = -triangleNormal[2];
    }
  
  // Computed the minimum value of k allowed by tolerance.
  kTolerance = (triangleRadiusSquared - this->Tolerance * this->Tolerance)
    / (2.0 * this->Tolerance);
  
  // Choose a large initial radius
  radiusSquared = this->MaximumRadius * this->MaximumRadius;
  if (radiusSquared < triangleRadiusSquared)
    {
    radiusSquared = triangleRadiusSquared;
    kMin = 0.0;
    if (kTolerance > 0.0)
      {
      return NULL;
      }
    }
  else
    {
    kMin = sqrt(radiusSquared - triangleRadiusSquared);
    }
  
  // Initialize the center
  center[0] = triangleCentroid[0] + kMin * triangleNormal[0];
  center[1] = triangleCentroid[1] + kMin * triangleNormal[1];
  center[2] = triangleCentroid[2] + kMin * triangleNormal[2];
  
  // Look through every point to reduce radius.
  numPoints = this->Points->GetNumberOfPoints();
  for (idxPoint = 0; idxPoint < numPoints; ++idxPoint)
    {
    this->Points->GetPoint(idxPoint, point);
    if (point != p0 && point != p1 && point != p2)
      {
      // Compute distance to this point.
      temp = center[0] - point[0]; dSquared = temp * temp;
      temp = center[1] - point[1]; dSquared += temp * temp;
      temp = center[2] - point[2]; dSquared += temp * temp;
      // If point is in sphere
      // Use slightly smaller sphere (but within tolerance)
      if (dSquared < kMin * kMin)
	{
	temp = this->ComputeNewCentroid(triangleCentroid, triangleNormal,
					triangleRadiusSquared, point);
	if (temp < 0.0)
	  {
	  // Because point is in current sphere
	  if (kTolerance > 0.0)
	    {
	    return NULL;
	    }
	  // Don't even bother looking through the rest of the points.
	  vtkDebugMacro(<< "Insphere (" << point[0] << ", " << point[1]
   	                << ", " << point[2] << ")");
	  return MakeSphere(p0, p1, p2, triangleCentroid, triangleNormal, 
			    triangleRadiusSquared, 0.0, triangleCentroid);
	  }
	if (temp < kMin)
	  {
	  kMin = temp;
	  // Check for tolerance violation
	  if (kMin < kTolerance)
	    {
	    return NULL;
	    }
	  d[0] = point[0]; d[1] = point[1]; d[2] = point[2];
	  // Compute new center and radiusSquared
	  radiusSquared = triangleRadiusSquared + kMin*kMin;
	  center[0] = triangleCentroid[0] + kMin * triangleNormal[0];
	  center[1] = triangleCentroid[1] + kMin * triangleNormal[1];
	  center[2] = triangleCentroid[2] + kMin * triangleNormal[2];
	  }
	}
      }
    }
  

  // Look through all new points as well.
  numPoints = this->NewPoints->GetNumberOfPoints();
  for (idxPoint = 0; idxPoint < numPoints; ++idxPoint)
    {
    this->NewPoints->GetPoint(idxPoint, point);
    if (point != p0 && point != p1 && point != p2)
      {
      // Compute distance to this point.
      temp = center[0] - point[0]; dSquared = temp * temp;
      temp = center[1] - point[1]; dSquared += temp * temp;
      temp = center[2] - point[2]; dSquared += temp * temp;
      // If point is in sphere
      // Use slightly smaller sphere (but within tolerance)
      if (dSquared < kMin * kMin)
	{
	temp = this->ComputeNewCentroid(triangleCentroid, triangleNormal,
					triangleRadiusSquared, point);
	if (temp < 0.0)
	  {
	  // Because point is in current sphere
	  if (kTolerance > 0.0)
	    {
	    return NULL;
	    }
	  // Don't even bother looking through the rest of the points.
	  vtkDebugMacro(<< "Insphere2 (" << point[0] << ", " << point[1]
   	                << ", " << point[2] << ")");
	  return MakeSphere(p0, p1, p2, triangleCentroid, triangleNormal, 
			    triangleRadiusSquared, 0.0, triangleCentroid);
	  }
	if (temp < kMin)
	  {
	  kMin = temp;
	  // Check for tolerance violation
	  if (kMin < kTolerance)
	    {
	    return NULL;
	    }
	  d[0] = point[0]; d[1] = point[1]; d[2] = point[2];
	  // Compute new center and radiusSquared
	  radiusSquared = triangleRadiusSquared + kMin*kMin;
	  center[0] = triangleCentroid[0] + kMin * triangleNormal[0];
	  center[1] = triangleCentroid[1] + kMin * triangleNormal[1];
	  center[2] = triangleCentroid[2] + kMin * triangleNormal[2];
	  }
	}
      }
    }
  
  // Sphere is ok
  vtkDebugMacro(<< "Limit (" << d[0] << ", " << d[1] << ", " << d[2] << ")");
  return this->MakeSphere(p0, p1, p2, triangleCentroid, triangleNormal, 
			  triangleRadiusSquared, kMin, center);
}


  
  
  
  
  
//----------------------------------------------------------------------------
vtkSphereTreeSphere *
vtkSphereTree::MakeSphere(float *p0, float *p1, float *p2,
			  float *triangleCentroid, float *triangleNormal, 
			  float triangleRadiusSquared, float kMin, 
			  float *center)
{
  vtkSphereTreeSphere * sphere;
  int idx;
  
  // Create a new sphere
  idx = sizeof(struct vtkSphereTreeSphere);
  sphere = (vtkSphereTreeSphere *)malloc(sizeof(struct vtkSphereTreeSphere));
  for (idx = 0; idx < 3; ++idx)
    {
    sphere->TriangleCentroid[idx] = triangleCentroid[idx];
    sphere->TriangleNormal[idx] = triangleNormal[idx];
    sphere->Center[idx] = center[idx];
    sphere->P0[idx] = p0[idx];
    sphere->P1[idx] = p1[idx];
    sphere->P2[idx] = p2[idx];
    }
  sphere->TriangleRadiusSquared = triangleRadiusSquared;
  sphere->K = kMin;
  sphere->RadiusSquared = triangleRadiusSquared + kMin * kMin;
  
  // Add this sphere to the list.
  sphere->Next = this->Spheres;
  this->Spheres = sphere;

  vtkDebugMacro(<< "MakeSphere: Radius = " << sqrt(sphere->RadiusSquared)
  << ", Tol = " << (sqrt(sphere->RadiusSquared) - kMin)
  << ", K = " << kMin << ", Norm = (" << triangleNormal[0] << ", "
  << triangleNormal[1] << ", " << triangleNormal[2] << "), Center = ("
  << center[0] << ", " << center[1] << ", "
  << center[2] << ")");

  return sphere;
}



//----------------------------------------------------------------------------
// This function returns the number of spheres
int vtkSphereTree::GetNumberOfSpheres()
{
  vtkSphereTreeSphere *sphere;
  int count = 0;
  
  sphere = this->Spheres;
  while (sphere)
    {
    ++count;
    sphere = sphere->Next;
    }
  
  return count;
}



//----------------------------------------------------------------------------
// This function gets rid of all the spheres and points.
void vtkSphereTree::CleanUp()
{
  vtkSphereTreeSphere *sphere;

  // Since we did not allocate points, ...
  this->Points = NULL;
  
  if (this->NewPoints)
    {
    this->NewPoints->Delete();
    this->NewPoints = NULL;
    }
  
  while (this->Spheres)
    {
    sphere = this->Spheres;
    this->Spheres = sphere->Next;
    free(sphere);
    }
}









